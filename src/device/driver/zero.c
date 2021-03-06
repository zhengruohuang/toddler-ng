#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <drv/drv.h>


/*
 * Ops
 */
static int dev_zero_read(devid_t id, void *buf, size_t count, size_t offset,
                         struct fs_file_op_result *result)
{
    if (count) {
        memzero(buf, count);
    }
    result->count = count;
    result->more = 1;
    return 0;
}

static int dev_zero_write(devid_t id, void *buf, size_t count, size_t offset,
                          struct fs_file_op_result *result)
{
    result->count = 0;
    return 0;
}


/*
 * Init
 */
static const struct drv_ops dev_zero_ops = {
    .read = dev_zero_read,
    .write = dev_zero_write,
};

void init_dev_zero()
{
    create_drv("/dev", "zero", 0, &dev_zero_ops, 1);
}
