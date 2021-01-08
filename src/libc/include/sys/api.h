#ifndef __LIBC_INCLUDE_SYS_API_H__
#define __LIBC_INCLUDE_SYS_API_H__


#include <stddef.h>


/*
 * System APIs
 */
enum system_api {
    SYS_API_PING = 16,

    // VFS
    SYS_API_ACQUIRE,
    SYS_API_RELEASE,

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
 * File system
 */
struct sys_dir_ent {
    short size; // size of this entry
    short type;
    unsigned long fs_id; // next read starts from the entry right after fs_id
    char name[];
};

// Acquire
extern int sys_api_acquire(const char *pathname);
extern int sys_api_release(int fd);

// File
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
