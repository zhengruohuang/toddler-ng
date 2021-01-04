#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic.h>
#include <assert.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>

#include "common/include/names.h"
#include "common/include/inttypes.h"
#include "libk/include/coreimg.h"
#include "system/include/vfs.h"


#define ROOT_FS_ID 999


/*
 * Response
 */
static inline msg_t *coreimgfs_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    return msg;
}


/*
 * Lookup
 */
static void coreimgfs_lookup(msg_t *msg)
{
    // Request
    ulong parent_fs_id = msg_get_param(msg, 1); // ent_fs_id
    char *name = msg_get_data(msg, 2, NULL);    // name

    // Lookup
    if (parent_fs_id != ROOT_FS_ID) {
        coreimgfs_response(-1);
        return;
    }

    int idx = coreimg_find_file_idx(name);
    if (idx == -1) {
        coreimgfs_response(-1);
        return;
    }

    //kprintf("CoreimgFS lookup @ %s\n", name);

    // Response
    msg = coreimgfs_response(0);            // ok
    msg_append_param(msg, idx + 1);         // ent_fs_id
    msg_append_int(msg, 1);                 // cacheable
}


/*
 * Acquire
 */
static void coreimgfs_acquire(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Acquire
    int idx = fs_id - 1;
    if (fs_id != ROOT_FS_ID &&
        (idx < 0 || idx >= coreimg_file_count()))
    {
        msg = coreimgfs_response(-1);
        return;
    }

    // Response
    msg = coreimgfs_response(0);            // ok
    msg_append_param(msg, fs_id);           // node_fs_id
    msg_append_int(msg, 1);                 // cacheable
}


/*
 * Release
 */
static void coreimgfs_release(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Release
    int idx = fs_id - 1;
    if (idx < 0 || idx >= coreimg_file_count()) {
        msg = coreimgfs_response(-1);
        return;
    }

    // Response
    msg = coreimgfs_response(0);               // ok
}


/*
 * File ops
 */
static void coreimgfs_file_open(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Open
    int idx = fs_id - 1;
    if (idx < 0 || idx >= coreimg_file_count()) {
        msg = coreimgfs_response(-1);
        return;
    }

    // Response
    msg = coreimgfs_response(0);            // ok
    msg_append_param(msg, fs_id);           // node_fs_id
}

static void coreimgfs_file_read(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset

    // Read
    int idx = fs_id - 1;
    if (idx < 0 || idx >= coreimg_file_count()) {
        coreimgfs_response(-1);
        return;
    }

    size_t size = 0;
    void *data = coreimg_get_file2(idx, &size);
    if (!data || !size) {
        coreimgfs_response(-1);
        return;
    }

    kprintf("File read, offset: %lu, size: %lu\n", offset, req_size);

    // Response
    msg = coreimgfs_response(0);               // ok

    size_t real_size = 0;
    if (offset < size) {
        real_size = size - offset;
        if (real_size > req_size) {
            real_size = req_size;
        }

        size_t max_size = msg_remain_data_size(msg) - 0x16ul; // extra safe
        if (real_size > max_size) {
            real_size = max_size;
        }
    }

    msg_append_param(msg, real_size);       // real size
    msg_append_data(msg, data && real_size ? data + offset : NULL,
                    real_size);             // data
}


/*
 * Dir ops
 */
static void coreimgfs_dir_open(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Open
    if (fs_id != ROOT_FS_ID) {
        coreimgfs_response(-1);
        return;
    }

    // Response
    msg = coreimgfs_response(0);               // ok
}

static void coreimgfs_dir_read(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);            // ent_fs_id
    ulong size = msg_get_param(msg, 2);             // size
    ulong last_fs_id = msg_get_param(msg, 3);       // offset

    // Read
    if (fs_id != ROOT_FS_ID) {
        coreimgfs_response(-1);
        return;
    }

    int start_idx = last_fs_id;

    kprintf("Dir read, start: %lu, max bytes: %lu\n", last_fs_id, size);

    // Response
    msg = coreimgfs_response(0);                // ok
    msg_append_param(msg, 0);                   // num entries

    ulong num_entries = 0;
    size_t copied_size = 0;
    for (int idx = start_idx; idx < coreimg_file_count(); idx++, num_entries++) {
        char name[32];
        coreimg_get_filename(idx, name, 32);

        size_t dent_size = sizeof(struct sys_dir_ent) + strlen(name) + 1;
        if (copied_size + dent_size > size) {
            break;
        }

        void *buf = msg_try_append_data(msg, NULL, dent_size);
        if (!buf) {
            break;
        }

        struct sys_dir_ent *dent = buf;
        dent->size = dent_size;
        dent->type = 0;
        dent->fs_id = idx + 1;
        strcpy(dent->name, name);
        //kprintf("copy: %s\n", name);

        copied_size += dent_size;
    }

    msg_set_param(msg, 1, num_entries);     // num entries
}


/*
 * Dispatcher
 */
static ulong coreimgfs_dispatch(ulong opcode)
{
    msg_t *msg = get_msg();
    ulong vfs_op = msg_get_param(msg, 0);
    kprintf("CoreimgFS op: %lu\n", vfs_op);
    switch (vfs_op) {
    case VFS_OP_LOOKUP:
        coreimgfs_lookup(msg);
        break;
    case VFS_OP_ACQUIRE:
        coreimgfs_acquire(msg);
        break;
    case VFS_OP_RELEASE:
        coreimgfs_release(msg);
        break;
    case VFS_OP_FILE_OPEN:
        coreimgfs_file_open(msg);
        break;
    case VFS_OP_FILE_READ:
        coreimgfs_file_read(msg);
        break;
    case VFS_OP_DIR_OPEN:
        coreimgfs_dir_open(msg);
        break;
    case VFS_OP_DIR_READ:
        coreimgfs_dir_read(msg);
        break;
    default:
        kprintf("Unknown VFS op: %lu\n", vfs_op);
        coreimgfs_response(-1);
        break;
    }

    syscall_ipc_respond();
    return 0;
}


/*
 * Init
 */
void init_coreimgfs()
{
    // Map and init core image
    void *ci = (void *)syscall_vm_map(VM_MAP_COREIMG, 0, 0);
    panic_if(!ci, "Unable to map core image!\n");

    init_libk_coreimg(ci);
    kprintf("Core image mapped @ %p, size: %u, num files: %u\n",
            ci, coreimg_size(), coreimg_file_count());

    // Mount
    const int coreimgfs_opcode = 129;
    const int coreimgfs_read_only = 1;
    const u32 coreimgfs_ignored_ops = ~(
        (0x1 << VFS_OP_LOOKUP)      |
        (0x1 << VFS_OP_ACQUIRE)     | (0x1 << VFS_OP_RELEASE)   |
        (0x1 << VFS_OP_FILE_OPEN)   | (0x1 << VFS_OP_FILE_READ) |
        (0x1 << VFS_OP_DIR_OPEN)    | (0x1 << VFS_OP_DIR_READ)
    );

    register_msg_handler(coreimgfs_opcode, coreimgfs_dispatch);

    struct ventry *ent = vfs_acquire("/sys/coreimg");
    vfs_mount(ent, "coreimgfs", syscall_get_tib()->pid, coreimgfs_opcode,
              ROOT_FS_ID, coreimgfs_read_only, coreimgfs_ignored_ops);
}
