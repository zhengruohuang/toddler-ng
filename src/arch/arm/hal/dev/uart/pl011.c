#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mem.h"


#define PL011_DATA_REG 0x0
#define PL011_FLAG_REG 0x18

struct arm_pl011_record {
    int reg_shift;
    ulong mmio_base;
};


/*
 * IO helpers
 */
static inline u16 arm_pl011_io_read(struct arm_pl011_record *record, int reg_sel)
{
    ulong addr = record->mmio_base + ((ulong)reg_sel << record->reg_shift);
    return mmio_read16(addr);
}

static inline void arm_pl011_io_write(struct arm_pl011_record *record, int reg_sel, u8 val)
{
    ulong addr = record->mmio_base + ((ulong)reg_sel << record->reg_shift);
    mmio_write8(addr, val);
}


/*
 * putchar
 */
static struct arm_pl011_record *pl011_putchar_record = NULL;

static int arm_pl011_putchar(int ch)
{
    u16 busy = 1;
    while (busy) {
        u16 flags = arm_pl011_io_read(pl011_putchar_record, PL011_FLAG_REG);
        busy = flags & 0x28;
    }

    arm_pl011_io_write(pl011_putchar_record, PL011_DATA_REG, ch);
    return 1;
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct arm_pl011_record *record = param->record;
    pl011_putchar_record = record;
}

static void create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct arm_pl011_record *record = mempool_alloc(sizeof(struct arm_pl011_record));
    memzero(record, sizeof(struct arm_pl011_record));
    param->record = record;

    u64 reg = 0, size = 0;
    int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
    panic_if(next, "arm,pl011 only supports one reg field!");

    paddr_t mmio_paddr = cast_u64_to_paddr(reg);
    ulong mmio_size = cast_paddr_to_vaddr(size);
    ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);
    record->mmio_base = mmio_vaddr;

    int reg_shift = devtree_get_reg_shift(fw_info->devtree_node);
    record->reg_shift = reg_shift;

    kprintf("Found ARM PrimeCell UART PL011 @ %lx, window @ %lx\n",
            mmio_paddr, mmio_vaddr);
}


static const char *arm_pl011_devtree_names[] = {
    "arm,pl011",
    NULL
};

DECLARE_SERIAL_DRIVER(arm_pl011, "ARM PrimeCell UART PL011",
                      arm_pl011_devtree_names, create, setup, arm_pl011_putchar);
