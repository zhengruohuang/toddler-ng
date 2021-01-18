#ifndef __LIBDRV_INCLUDE_DRV_H__
#define __LIBDRV_INCLUDE_DRV_H__


#include <stddef.h>
#include <fs/fs.h>


typedef unsigned long devid_t;

struct drv_ops {
    int (*read)(devid_t id, void *buf, size_t count, size_t offset,
                struct fs_file_op_result *result);
    int (*write)(devid_t id, void *buf, size_t count, size_t offset,
                 struct fs_file_op_result *result);
};


int create_drv(const char *path, const char *name, devid_t devid, const struct drv_ops *ops, int popup);


#endif
