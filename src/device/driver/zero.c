#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <hal.h>
#include <fs/fs.h>
#include "device/include/devfs.h"


/*
 * Dispatch
 */
static inline msg_t *dispatch_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    return msg;
}

static inline void dispatch_file_read(msg_t *msg)
{
    // Request
    __unused_var ulong devid = msg_get_param(msg, 1);    // devid
    ulong req_size = msg_get_param(msg, 2); // size
    __unused_var ulong offset = msg_get_param(msg, 3);   // offset

    // Response
    msg = dispatch_response(0);             // ok
    msg_append_param(msg, 0);               // real size

    // Buffer
    size_t max_size = msg_remain_data_size(msg) - 32ul; // extra safe
    if (req_size > max_size) req_size = max_size;
    void *buf = msg_append_data(msg, NULL, req_size);   // data

    // Read
    memzero(buf, req_size);
    msg_set_param(msg, 1, req_size);
}

static inline void dispatch_file_write(msg_t *msg)
{
    // Response
    msg = dispatch_response(0);             // ok
}

static unsigned long dispatch(unsigned long opcode)
{
    msg_t *msg = get_msg();
    ulong vfs_op = msg_get_param(msg, 0);
    kprintf("%s op: %lu\n", "/dev/zero", vfs_op);

    switch (vfs_op) {
    case VFS_OP_FILE_READ:
        dispatch_file_read(msg);
        break;
    case VFS_OP_FILE_WRITE:
        dispatch_file_write(msg);
        break;
    default:
        kprintf("Unknown VFS op: %lu\n", vfs_op);
        dispatch_response(-1);
        break;
    }

    syscall_ipc_respond();
    return 0;
}


void init_dev_zero()
{
    register_msg_handler(64, dispatch);
    devfs_register("zero", 0, syscall_get_tib()->pid, 64);
}
