#ifndef __DEVICE_INCLUDE_DEVFS_H__
#define __DEVICE_INCLUDE_DEVFS_H__


#include <sys.h>
#include <fs/pseudo.h>


struct devfs_ipc {
    pid_t pid;
    unsigned long opcode;

    unsigned long devid;
    void *buf;
    size_t count;
    size_t offset;

    int err;
    struct fs_file_op_result *result;
};


extern void init_devfs();


#endif
