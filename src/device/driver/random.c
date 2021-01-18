#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <drv/drv.h>


/*
 * Ops
 */
static unsigned int rand_seed = 1;

static int dev_rand_read(devid_t id, void *buf, size_t count, size_t offset,
                         struct fs_file_op_result *result)
{
    if (count) {
        unsigned char *bytes = buf;
        for (size_t i = 0; i < count; i++) {
            bytes[i] = rand_r(&rand_seed) & 0xff;
        }
    }

    result->count = count;
    result->more = 1;
    return 0;
}

static int dev_rand_write(devid_t id, void *buf, size_t count, size_t offset,
                          struct fs_file_op_result *result)
{
    result->count = 0;
    return 0;
}


/*
 * Init
 */
static const struct drv_ops dev_rand_ops = {
    .read = dev_rand_read,
    .write = dev_rand_write,
};

void init_dev_random()
{
    create_drv("/dev", "random", 0, &dev_rand_ops, 1);
}
