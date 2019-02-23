#include "common/include/inttypes.h"
#include "loader/include/atags.h"
#include "loader/include/devtree.h"
#include "loader/include/lib.h"


static struct atag *atags = NULL;


static inline struct atag *next_atag(struct atag *cur)
{
    if (!cur || cur->tag == ATAG_NONE) {
        return NULL;
    }

    struct atag *next = (void *)cur + (cur->size << 2);
    return next->tag != ATAG_NONE ? next : NULL;
}

static void build_chosen_node()
{
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        chosen = devtree_alloc_node(devtree_get_root(), "chosen");
    }

    for (struct atag *atag = atags; atag; atag = next_atag(atag)) {
        if (atag->size == 2) {
            continue;
        }

        switch (atag->tag) {
        case ATAG_CORE:
            devtree_alloc_prop_u32(chosen, "pagesize", atag->core.pagesize);
            break;
        case ATAG_MEM:
            devtree_alloc_prop_u64(chosen, "memstart", atag->mem.start);
            devtree_alloc_prop_u64(chosen, "memsize", atag->mem.size);
            break;
        case ATAG_VIDEOTEXT:
            break;
        case ATAG_RAMDISK:
            devtree_alloc_prop_u64(chosen, "ramdisk-start",
                atag->ramdisk.start);
            devtree_alloc_prop_u64(chosen, "ramdisk-end",
                atag->ramdisk.start + atag->ramdisk.size);
            break;
        case ATAG_INITRD2:
            devtree_alloc_prop_u64(chosen, "initrd-start",
                atag->initrd2.start);
            devtree_alloc_prop_u64(chosen, "initrd-end",
                atag->initrd2.start + atag->initrd2.size);
            break;
        case ATAG_SERIAL:
            devtree_alloc_prop_u64(chosen, "machine-serial",
                ((u64)atag->serial.high << 32) | atag->serial.low);
            break;
        case ATAG_REVISION:
            devtree_alloc_prop_u32(chosen, "machine-revision", atag->rev);
            break;
        case ATAG_VIDEOLFB:
            break;
        case ATAG_CMDLINE:
            devtree_alloc_prop(chosen, "bootargs",
                &atag->cmdline, (atag->size - 2) << 2);
            break;
        default:
            panic("Unknown ATAG: %x\n", atag->tag);
            break;
        }
    }
}

void init_atags(void *fw_atags)
{
    atags = fw_atags;
    build_chosen_node();
}
