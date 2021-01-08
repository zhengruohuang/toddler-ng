#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <sys.h>
#include <sys/api.h>
#include "libk/include/mem.h"


/*
 * Acquire and release
 */
int sys_api_acquire(const char *pathname)
{
    msg_t *msg = get_empty_msg();
    msg_append_str(msg, pathname, 0);
    syscall_ipc_popup_request(0, SYS_API_ACQUIRE);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    int fd = msg_get_int(msg, 1);
    return fd;
}

int sys_api_release(int fd)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, fd);
    syscall_ipc_popup_request(0, SYS_API_RELEASE);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}


/*
 * File fead and write
 */
int sys_api_file_open(int fd, unsigned int flags, unsigned int mode)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, fd);
    msg_append_param(msg, flags);
    msg_append_param(msg, mode);
    syscall_ipc_popup_request(0, SYS_API_FILE_OPEN);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}

ssize_t sys_api_file_read(int fd, void *buf, size_t count, size_t offset)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, fd);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    syscall_ipc_popup_request(0, SYS_API_FILE_READ);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    size_t read_count = msg_get_param(msg, 1);
    panic_if(read_count > count,
             "VFS read returned (%lu) more than requested (%lu)!\n",
             read_count, count);

    void *read_data = msg_get_data(msg, 2, NULL);
    memcpy(buf, read_data, read_count);

    return read_count;
}

ssize_t sys_api_file_write(int fd, const void *buf, size_t count, size_t offset)
{
    panic("not implemented!\n");
    return -1;
}


/*
 * Dir
 */
int sys_api_dir_open(int fd, unsigned int flags)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, fd);
    msg_append_param(msg, flags);
    syscall_ipc_popup_request(0, SYS_API_DIR_OPEN);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}

ssize_t sys_api_dir_read(int fd, struct sys_dir_ent *dirp, size_t count,
                         unsigned long offset)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, fd);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    syscall_ipc_popup_request(0, SYS_API_DIR_READ);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    size_t num_entries = msg_get_param(msg, 1);
    size_t read_count = 0;

    for (size_t i = 0, pidx = 2; i < num_entries; i++, pidx++) {
        size_t ent_size = 0;
        struct sys_dir_ent *vfs_dirp = msg_get_data(msg, (int)pidx, &ent_size);

        if (ent_size <= offsetof(struct sys_dir_ent, name)) {
            return -1;
        }
        if (read_count + ent_size > count) {
            break;
        }

        memcpy(dirp, vfs_dirp, vfs_dirp->size);
        ((char *)(void *)dirp)[vfs_dirp->size - 1] = '\0';
        dirp->size = ent_size;

        read_count += ent_size;
        dirp = (void *)dirp + ent_size;
    }

    return read_count;
}

int sys_api_dir_create(int dirfd, const char *pathname, unsigned int mode)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, dirfd);
    msg_append_str(msg, pathname, 0);
    msg_append_param(msg, mode);
    syscall_ipc_popup_request(0, SYS_API_DIR_CREATE);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    int new_fd = msg_get_int(msg, 1);
    return new_fd;
}

int sys_api_dir_remove(int dirfd)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, dirfd);
    syscall_ipc_popup_request(0, SYS_API_DIR_REMOVE);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}


/*
 * Mount and unmount
 */
int sys_api_mount(int fd, const char *name,
                  unsigned long opcode, unsigned long ops_ignore_map)
{
    ulong pid = syscall_get_tib()->pid;

    msg_t *msg = get_empty_msg();
    msg_append_int(msg, fd);
    msg_append_str(msg, name, 0);
    msg_append_param(msg, pid);
    msg_append_param(msg, opcode);
    msg_append_param(msg, ops_ignore_map);
    syscall_ipc_popup_request(0, SYS_API_MOUNT);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}
