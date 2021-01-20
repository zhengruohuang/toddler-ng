#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>

#include "common/include/inttypes.h"
#include "libk/include/mem.h"
#include "system/include/vfs.h"
#include "system/include/task.h"


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
 * Special node creation
 */
static ulong vfs_api_node_create(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int dirfd = msg_get_int(msg, 0);
    const char *name = msg_get_data(msg, 1, NULL);
    ulong flags = msg_get_param(msg, 2);
    ulong node_type = msg_get_param(msg, 3);
    pid_t pid = msg->sender.pid;

    // Obtain containing dir
    struct ventry *vent = task_lookup_fd(pid, dirfd);
    if (!vent || !vent->vnode) {
        _err_response(-1);
    }

    // Create node
    int err = -1;
    switch (node_type) {
    case VFS_NODE_DEV: {
        pid_t drv_pid = msg_get_param(msg, 4);
        unsigned long drv_opcode = msg_get_param(msg, 5);
        err = vfs_dev_create(vent->vnode, name, flags, drv_pid, drv_opcode);
        break;
    }
    case VFS_NODE_PIPE: {
        err = vfs_pipe_create(vent->vnode, name, flags);
        break;
    }
    default:
        break;
    }

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);

    syscall_ipc_respond();
    return 0;
}


/*
 * Acquire and release
 */
static ulong vfs_api_acquire(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    char *pathname = strdup((char *)msg_get_data(msg, 0, NULL));
    pid_t pid = msg->sender.pid;

    // Acquire
    struct ventry *vent = vfs_acquire(pathname);
    free(pathname);

    if (!vent || !vent->vnode) {
        _err_response(-1);
    }

    // Set up file descriptor
    int fd = task_alloc_fd(pid, vent);
    //int fd = alloc_fd();
    //fd_table[fd] = vent;

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
    pid_t pid = msg->sender.pid;

    // Release
    //struct ventry *vent = lookup_fd(fd);
    struct ventry *vent = task_lookup_fd(pid, fd);
    int err = vfs_release(vent);
    if (!err) {
        //free_fd(fd);
        task_free_fd(pid, fd);
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
    pid_t pid = msg->sender.pid;

    // Open file
    //struct ventry *vent = lookup_fd(fd);
    struct ventry *vent = task_lookup_fd(pid, fd);
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
    pid_t pid = msg->sender.pid;

    // Read
    //struct ventry *vent = lookup_fd(fd);
    struct ventry *vent = task_lookup_fd(pid, fd);
    vfs_file_read_forward(vent->vnode, count, offset);

    // Response
    syscall_ipc_respond();
    return 0;
}

static ulong vfs_api_file_write(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);
    size_t count = msg_get_param(msg, 1);
    size_t offset = msg_get_param(msg, 2);
    void *data = msg_get_data(msg, 3, NULL);
    pid_t pid = msg->sender.pid;

    // Duplicate data
    void *data_dup = malloc(count);
    memcpy(data_dup, data, count);

    // Write
    struct ventry *vent = task_lookup_fd(pid, fd);
    ssize_t wc = vfs_file_write(vent->vnode, data_dup, count, offset);
    free(data_dup);

    // Response
    msg = get_empty_msg();
    if (wc < 0) {
        msg_append_int(msg, (int)wc);
    } else {
        msg_append_int(msg, 0);
        msg_append_param(msg, (size_t)wc);
    }

    syscall_ipc_respond();
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
    pid_t pid = msg->sender.pid;

    // Open dir
    //struct ventry *vent = lookup_fd(fd);
    struct ventry *vent = task_lookup_fd(pid, fd);
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
    pid_t pid = msg->sender.pid;

    // Read
    //struct ventry *vent = lookup_fd(fd);
    struct ventry *vent = task_lookup_fd(pid, fd);
    vfs_dir_read_forward(vent->vnode, count, offset);

    // Response
    syscall_ipc_respond();
    return 0;
}


/*
 * Mount
 */
static ulong vfs_api_mount(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int fd = msg_get_int(msg, 0);
    char *name = strdup(msg_get_data(msg, 1, NULL));
    pid_t pid = msg_get_param(msg, 2);
    unsigned long op = msg_get_param(msg, 3);
    unsigned long ops_ignore_map = msg_get_param(msg, 4);
    pid_t sender_pid = msg->sender.pid;

    // Mount
    //struct ventry *vent = fd ? lookup_fd(fd) : NULL;
    struct ventry *vent = fd ? task_lookup_fd(sender_pid, fd) : NULL;
    vfs_mount(vent, name, pid, op, ops_ignore_map);
    free(name);

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

    register_msg_handler(SYS_API_NODE_CREATE, vfs_api_node_create);

    register_msg_handler(SYS_API_FILE_OPEN, vfs_api_file_open);
    register_msg_handler(SYS_API_FILE_READ, vfs_api_file_read);
    register_msg_handler(SYS_API_FILE_WRITE, vfs_api_file_write);

    register_msg_handler(SYS_API_DIR_OPEN, vfs_api_dir_open);
    register_msg_handler(SYS_API_DIR_READ, vfs_api_dir_read);

    register_msg_handler(SYS_API_MOUNT, vfs_api_mount);
}
