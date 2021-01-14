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
    // Response
    msg = dispatch_response(0);             // ok
    msg_append_param(msg, 0);               // real size
    msg_append_data(msg, NULL, 0);          // empty data
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
    kprintf("%s op: %lu\n", "/dev/null", vfs_op);

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


void init_dev_null()
{
    register_msg_handler(65, dispatch);
    devfs_register("null", 0, syscall_get_tib()->pid, 65);
}
