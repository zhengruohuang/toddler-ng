#include <atomic.h>
#include <string.h>
#include <assert.h>
#include <sys.h>
#include <sys/api.h>
#include <fs/fs.h>
#include <drv/drv.h>

#include "device/include/devfs.h"


#define DRV_IPC_OPCODE_BASE (128 + 64)

struct drv_record {
    void *fs;
    struct drv_ops ops;

    const char *path;
    const char *name;

    unsigned long ignored_ops_map;
    int read_only;
};


/*
 * Driver dispatch index
 */
// Maximum number of drivers a process can register
#define MAX_NUM_DRV 32

static unsigned long drv_idx = 0;
static struct drv_record drv_records[MAX_NUM_DRV] = { };

static unsigned long alloc_drv_idx()
{
    unsigned long idx = atomic_fetch_and_add(&drv_idx, 1);
    panic_if(idx >= MAX_NUM_DRV, "Too many drivers in a process!\n");
    return idx;
}


/*
 * Dispatch operations
 */
static inline msg_t *dispatch_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    return msg;
}

static inline void dispatch_file_read(msg_t *msg, struct drv_record *record)
{
    // Request
    ulong devid = msg_get_param(msg, 1);    // devid
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset

    // Response
    msg = dispatch_response(0);             // ok
    msg_append_param(msg, 0);               // real size
    msg_append_int(msg, 0);                 // more

    // Buffer
    size_t max_size = msg_remain_data_size(msg) - 32ul; // extra safe
    if (req_size > max_size) req_size = max_size;
    void *buf = msg_append_data(msg, NULL, req_size);   // data

    //kprintf("DRV read req_size: %lu, offset: %lu\n", req_size, offset);

    // Read
    int err = 0;
    if (record->ops.read) {
        struct fs_file_op_result result = { };
        err = record->ops.read(devid, buf, req_size, offset, &result);

        if (!err) {
            msg_set_param(msg, 1, result.count);
            //msg_set_int(msg, 2, result.more);
        }
    }

    // Response
    msg_set_int(msg, 0, err);
}

static inline void dispatch_file_write(msg_t *msg, struct drv_record *record)
{
    // Request
    ulong devid = msg_get_param(msg, 1);    // devid
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset
    void *data = msg_get_data(msg, 4, NULL);// data

    //kprintf("DRV write, req_size: %lu, offset: %lu\n", req_size, offset);

    // Write
    int err = 0;
    size_t write_size = 0;
    if (record->ops.write) {
        struct fs_file_op_result result = { };
        err = record->ops.write(devid, data, req_size, offset, &result);

        if (!err) {
            write_size = result.count;
        }
    }

    // Response
    msg = dispatch_response(err);           // err
    if (!err) {
        msg_append_param(msg, write_size);  // real size
    }
}


/*
 * Dispatch
 */
static unsigned long dispatch(unsigned long opcode)
{
    int idx = opcode - DRV_IPC_OPCODE_BASE;
    struct drv_record *record = &drv_records[idx];

    msg_t *msg = get_msg();
    ulong vfs_op = msg_get_param(msg, 0);
    //kprintf("%s op: %lu\n", record->name, vfs_op);

    switch (vfs_op) {
    case VFS_OP_MOUNT:
    case VFS_OP_FILE_READ:
        dispatch_file_read(msg, record);
        break;
    case VFS_OP_FILE_WRITE:
        dispatch_file_write(msg, record);
        break;
    default:
        kprintf("Unknown VFS op: %lu in %s\n", vfs_op, record->name);
        dispatch_response(-1);
        break;
    }

    syscall_ipc_respond();
    return 0;
}


/*
 * Register a new driver
 */
int create_drv(const char *path, const char *name, devid_t devid, const struct drv_ops *ops, int popup)
{
    unsigned long idx = alloc_drv_idx();
    unsigned long opcode = DRV_IPC_OPCODE_BASE + idx;

    struct drv_record *record = &drv_records[idx];
    memzero(record, sizeof(struct drv_record));
    record->path = strdup(path ? path : "/");
    record->name = strdup(name);
    memcpy(&record->ops, ops, sizeof(struct drv_ops));

    if (popup) {
        register_msg_handler(opcode, dispatch);
    } else {
        // TODO: serial handler
    }

    int fd = path ? sys_api_acquire(path) : 0;
    panic_if(path && fd <= 0, "Unable to register driver!\n");

    pid_t pid = syscall_get_tib()->pid;
    int err = sys_api_dev_create(fd, name, 0, pid, opcode);

    kprintf("Driver created, opcode: %lu\n", opcode);

    return err;
}
