#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <kth.h>
#include <sys.h>
#include <fs/pseudo.h>

#include "device/include/devfs.h"


/*
 * FS structure
 */
enum devfs_node_type {
    DEVFS_NODE_DIR,
    DEVFS_NODE_DEV,
};

struct devfs_node {
    struct pseudo_fs_node fs_node;
    int type;
    unsigned long devid;

    pid_t pid;
    unsigned long opcode;
};

struct devfs {
    struct pseudo_fs fs_node;
};

static salloc_obj_t devfs_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct devfs_node));

static struct devfs devfs;
static struct devfs_node *devfs_root = NULL;


static struct devfs_node *new_devfs_node(struct devfs_node *parent, const char *name, int type)
{
    int node_type = type == DEVFS_NODE_DEV ? PSEUDO_NODE_FILE : PSEUDO_NODE_DIR;
    struct devfs_node *node = salloc(&devfs_salloc);
    node->type = type;

    pseudo_fs_node_setup(&devfs.fs_node, &node->fs_node, name,
                         node_type, 0, 0, 0);
    pseudo_fs_node_attach(&devfs.fs_node, &parent->fs_node, &node->fs_node);

    return node;
}

static struct devfs_node *new_device_node(struct devfs_node *parent,
                                          const char *name, unsigned long devid,
                                          pid_t pid, unsigned long opcode)
{
    struct devfs_node *node = new_devfs_node(parent, name, DEVFS_NODE_DEV);
    node->pid = pid;
    node->opcode = opcode;
    node->devid = devid;
    return node;
}

static struct devfs_node *setup_root()
{
    return new_devfs_node(NULL, "/", DEVFS_NODE_DIR);
}


/*
 * Device operations
 */
// TODO: needs rework
int devfs_register(const char *name, unsigned long devid, pid_t pid, unsigned long opcode)
{
    struct devfs_node *node = new_device_node(devfs_root, name, devid, pid, opcode);
    return node ? 0 : -1;
}

static unsigned long devfs_read_worker(unsigned long param)
{
    struct devfs_ipc *ipc_info = (void *)param;

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FILE_READ);
    msg_append_param(msg, ipc_info->devid);
    msg_append_param(msg, ipc_info->count);
    msg_append_param(msg, ipc_info->offset);

    // Fixup count
    size_t req_count = ipc_info->count;
    size_t max_size = msg_remain_data_size(msg) - 32ul; // extra safe
    if (req_count > max_size) req_count = max_size;

    // Send
    syscall_ipc_popup_request(ipc_info->pid, ipc_info->opcode);

    // Response
    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        ipc_info->err = err;
        return 0;
    }

    size_t read_count = msg_get_param(msg, 1);
    panic_if(read_count > req_count,
             "VFS read returned (%lu) more than requested (%lu)!\n",
             read_count, req_count);

    if (ipc_info->buf) {
        void *read_data = msg_get_data(msg, 2, NULL);
        memcpy(ipc_info->buf, read_data, read_count);
    }

    ipc_info->result->count = read_count;
}

static int devfs_read(void *fs, fs_id_t id, void *buf, size_t count,
                      size_t offset, struct fs_file_op_result *result)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *pfs_node = pseudo_fs_node_find(fs, id);
    if (!pfs_node) {
        err = -1;
        goto done;
    }

    struct devfs_node *node = container_of(pfs_node, struct devfs_node, fs_node);
    if (node->type != DEVFS_NODE_DEV) {
        err = -1;
        goto done;
    }

    kth_t sender;
    struct devfs_ipc ipc_info = {
        .pid = node->pid, .opcode = node->opcode, .devid = node->devid,
        .buf = buf, .count = count, .offset = offset,
        .err = 0, .result = result,
    };

    kprintf("here, pid: %lu, opcode: %lu\n", ipc_info.pid, ipc_info.opcode);

    kth_create(&sender, devfs_read_worker, (unsigned long)&ipc_info);
    kth_join(&sender, NULL);

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

static int devfs_write(void *fs, fs_id_t id, void *buf, size_t count,
                       size_t offset, struct fs_file_op_result *result)
{
    return -1;
}




/*
 * Ops
 */
static const struct fs_ops devfs_ops = {
    .mount = pseudo_fs_mount,
    .unmount = pseudo_fs_unmount,

    .lookup = pseudo_fs_lookup,

    .acquire = pseudo_fs_acquire,
    .release = pseudo_fs_release,

    .file_open = pseudo_fs_file_open,
    .file_read = devfs_read,
    .file_write = devfs_write,

    .dir_open = pseudo_fs_dir_open,
    .dir_read = pseudo_fs_dir_read,
};


/*
 * Init
 */
void init_devfs()
{
    kprintf("Mouting devfs\n");

    // Construct FS structure
    devfs_root = setup_root();

    // Mount
    create_fs("/dev", "devfs", &devfs.fs_node, &devfs_ops, 1);
}
