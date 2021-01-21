#ifndef __LIBC_INCLUDE_SYS_API_H__
#define __LIBC_INCLUDE_SYS_API_H__


#include <stddef.h>
#include <sys.h>


/*
 * System APIs
 */
enum system_api {
    SYS_API_PING = 16,

    // Task
    SYS_API_TASK_CREATE,
    SYS_API_EXIT,
    SYS_API_DETACH,
    SYS_API_WAIT,

    // VFS
    SYS_API_ACQUIRE,
    SYS_API_RELEASE,

    SYS_API_NODE_CREATE,

    SYS_API_FILE_CREATE,
    SYS_API_FILE_OPEN,
    SYS_API_FILE_READ,
    SYS_API_FILE_WRITE,

    SYS_API_DIR_OPEN,
    SYS_API_DIR_READ,
    SYS_API_DIR_CREATE,
    SYS_API_DIR_REMOVE,

    SYS_API_MOUNT,
    SYS_API_UNMOUNT,
};


/*
 * Task
 */
extern int sys_api_task_exit(unsigned long status);
extern int sys_api_task_detach(unsigned long status);
extern int sys_api_task_wait(pid_t pid, unsigned long *status);


/*
 * File system
 */
enum vfs_node_type {
    VFS_NODE_DUMMY,

    VFS_NODE_FILE,
    VFS_NODE_DIR,

    VFS_NODE_DEV,
    VFS_NODE_PIPE,
};

struct sys_dir_ent {
    short size; // size of this entry
    short type;
    unsigned long fs_id; // next read starts from the entry right after fs_id
    char name[];
};

// Acquire
extern int sys_api_acquire(const char *pathname);
extern int sys_api_release(int fd);

// Node
extern int sys_api_dev_create(int dirfd, const char *pathname, unsigned int mode, pid_t pid, unsigned long opcode);
extern int sys_api_pipe_create(int dirfd, const char *pathname, unsigned int mode);

// File
extern int sys_api_file_create(int dirfd, const char *pathname, unsigned int mode);
extern int sys_api_file_open(int fd, unsigned int flags, unsigned int mode);
extern ssize_t sys_api_file_read(int fd, void *buf, size_t count, size_t offset);
extern ssize_t sys_api_file_write(int fd, const void *buf, size_t count, size_t offset);

// Dir
extern int sys_api_dir_open(int fd, unsigned int flags);
extern ssize_t sys_api_dir_read(int fd, struct sys_dir_ent *dirp, size_t count, unsigned long offset);
extern int sys_api_dir_create(int dirfd, const char *pathname, unsigned int mode);
extern int sys_api_dir_remove(int dirfd);

// Mount
extern int sys_api_mount(int fd, const char *name,
                         unsigned long opcode, unsigned long ops_ignore_map);


#endif
