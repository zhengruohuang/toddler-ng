#include "common/include/inttypes.h"
#include "common/include/coreimg.h"
#include "loader/include/devtree.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"


/*
 * This symbol is declared as weak so that the actual coreimg can override this
 * The bin2c tool is used to convert the coreimg binary to C, and later compiled
 * to the loader ELF
 */
unsigned char __attribute__((weak, aligned(8))) embedded_coreimg[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};


static struct coreimg_fat *coreimg = NULL;


static bool is_coreimg_header(struct coreimg_fat *img)
{
    return img->header.magic == COREIMG_MAGIC;
}

void *find_supplied_coreimg(int *size)
{
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        return NULL;
    }

    struct devtree_prop *rd_start = devtree_find_prop(chosen, "initrd-start");
    if (!rd_start) {
        rd_start = devtree_find_prop(chosen, "linux,initrd-start");
        if (!rd_start) {
            return NULL;
        }
    }

    struct devtree_prop *rd_end = devtree_find_prop(chosen, "initrd-end");
    if (!rd_end) {
        rd_end = devtree_find_prop(chosen, "linux,initrd-end");
        if (!rd_end) {
            return NULL;
        }
    }

    u64 start = rd_start->len == 4 ? devtree_get_prop_data_u32(rd_start) :
                                     devtree_get_prop_data_u64(rd_start);
    u64 end = rd_end->len == 4 ? devtree_get_prop_data_u32(rd_end) :
                                 devtree_get_prop_data_u64(rd_end);
    if (end <= start) {
        return NULL;
    }

    if (!is_coreimg_header((void *)(ulong)start)) {
        return NULL;
    }

    if (size) {
        *size = (int)(end - start);
    }

    kprintf("supplied coreimg @ %llx, end @ %llx\n", start, end);

    return (void *)(ulong)start;
}

static void *search_appended_coreimg()
{
    if (is_coreimg_header((struct coreimg_fat *)embedded_coreimg)) {
        return embedded_coreimg;
    }

    return NULL;
}

void init_coreimg()
{
    coreimg = find_supplied_coreimg(NULL);
    if (!coreimg) {
        coreimg = search_appended_coreimg();
    }

    panic_if(!coreimg, "Unable to find coreimg!");
}

void *coreimg_find_file(const char *name)
{
    int file_count = coreimg->header.file_count;
    struct coreimg_record *record = NULL;

    for (int i = 0; i < file_count; i++) {
        record = &coreimg->records[i];
        if (!strncmp(name, record->file_name, 20)) {
            int offset = record->start_offset;
            return (void *)coreimg + offset;
        }
    }

    return NULL;
}
