#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <drv/drv.h>


/*
 * Ops
 */
static int dev_full_read(devid_t id, void *buf, size_t count, size_t offset,
                         struct fs_file_op_result *result)
{
    if (count) {
        memzero(buf, count);
    }
    result->count = count;
    result->more = 1;
    return 0;
}

static int dev_full_write(devid_t id, void *buf, size_t count, size_t offset,
                          struct fs_file_op_result *result)
{
    // FIXME: return enospc
    result->count = 0;
    return 0;
}


/*
 * Init
 */
static const struct drv_ops dev_full_ops = {
    .read = dev_full_read,
    .write = dev_full_write,
};

void init_dev_full()
{
    create_drv("/dev", "full", 0, &dev_full_ops, 1);
}
