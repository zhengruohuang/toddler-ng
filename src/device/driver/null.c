#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <drv/drv.h>


/*
 * Ops
 */
static int dev_null_read(devid_t id, void *buf, size_t count, size_t offset,
                         struct fs_file_op_result *result)
{
    result->count = 0;
    result->more = 0;
    return 0;
}

static int dev_null_write(devid_t id, void *buf, size_t count, size_t offset,
                          struct fs_file_op_result *result)
{
    result->count = 0;
    return 0;
}


/*
 * Init
 */
static const struct drv_ops dev_null_ops = {
    .read = dev_null_read,
    .write = dev_null_write,
};

void init_dev_null()
{
    create_drv("/dev", "null", 0, &dev_null_ops, 1);
}
