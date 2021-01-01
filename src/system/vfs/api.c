#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>

#include "common/include/inttypes.h"
#include "libk/include/mem.h"
#include "system/include/vfs.h"


/*
 * Fd table
 */
static volatile int fd_count = 4;
static struct ventry *fd_table[128] = { };

static int alloc_fd()
{
    panic_if(fd_count >= 128, "Too many file descriptors!\n");
    return fd_count++;
}

static struct ventry *lookup_fd(int fd)
{
    panic_if(fd >= 128, "Invalid file descriptor!\n");
    return fd_table[fd];
}

static void free_fd(int fd)
{
    panic_if(fd >= 128, "Invalid file descriptor!\n");
    fd_table[fd] = NULL;
}


/*
 * Error response
 */
static inline void _err_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    syscall_ipc_respond();
}


/*
 * Acquire and release
 */
static ulong vfs_api_acquire(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    char *pathname = strdup((char *)msg_get_data(msg, 0, NULL));

    // Acquire
    struct ventry *vent = vfs_acquire(pathname);
    free(pathname);

    if (!vent || !vent->vnode) {
        _err_response(-1);
    }

    // Set up file descriptor
    int fd = alloc_fd();
    fd_table[fd] = vent;

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, 0);
    msg_append_int(msg, fd);

    syscall_ipc_respond();
    return 0;
}

static ulong vfs_api_release(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);

    // Release
    struct ventry *vent = lookup_fd(fd);
    int err = vfs_release(vent);
    if (!err) {
        free_fd(fd);
    }

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);

    syscall_ipc_respond();
    return 0;
}


/*
 * File
 */
static ulong vfs_api_file_open(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);
    ulong flags = msg_get_param(msg, 1);
    ulong mode = msg_get_param(msg, 2);

    // Open file
    struct ventry *vent = lookup_fd(fd);
    int err = vfs_file_open(vent->vnode, flags, mode);

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);

    syscall_ipc_respond();
    return 0;
}

static ulong vfs_api_file_read(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);
    size_t count = msg_get_param(msg, 1);
    size_t offset = msg_get_param(msg, 2);

    // Read
    struct ventry *vent = lookup_fd(fd);
    vfs_file_read_forward(vent->vnode, count, offset);

    // Response
    syscall_ipc_respond();
    return 0;
}

static ulong vfs_api_file_write(ulong opcode)
{
    return 0;
}


/*
 * Dir
 */
static ulong vfs_api_dir_open(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);
    ulong flags = msg_get_param(msg, 1);

    // Open dir
    struct ventry *vent = lookup_fd(fd);
    int err = vfs_dir_open(vent->vnode, flags);

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);

    syscall_ipc_respond();
    return 0;
}

static ulong vfs_api_dir_read(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);
    size_t count = msg_get_param(msg, 1);
    size_t offset = msg_get_param(msg, 2);

    // Read
    struct ventry *vent = lookup_fd(fd);
    vfs_dir_read_forward(vent->vnode, count, offset);

    // Response
    syscall_ipc_respond();
    return 0;
}


/*
 * Init
 */
void init_vfs_api()
{
    register_msg_handler(SYS_API_ACQUIRE, vfs_api_acquire);
    register_msg_handler(SYS_API_RELEASE, vfs_api_release);

    register_msg_handler(SYS_API_FILE_OPEN, vfs_api_file_open);
    register_msg_handler(SYS_API_FILE_READ, vfs_api_file_read);
    register_msg_handler(SYS_API_FILE_WRITE, vfs_api_file_write);

    register_msg_handler(SYS_API_DIR_OPEN, vfs_api_dir_open);
    register_msg_handler(SYS_API_DIR_READ, vfs_api_dir_read);
}
