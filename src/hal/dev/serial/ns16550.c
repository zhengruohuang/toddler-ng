#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/mem.h"


#define UART_DATA_REG           (0x0)
#define UART_INT_ENABLE_REG     (0x1)
#define UART_INT_ID_FIFO_REG    (0x2)
#define UART_LINE_CTRL_REG      (0x3)
#define UART_MODEM_CTRL_REG     (0x4)
#define UART_LINE_STAT_REG      (0x5)
#define UART_MODEM_STAT_REG     (0x6)
#define UART_SCRATCH_REG        (0x7)

#define UART_DIV_LSB_REG        (0x0)
#define UART_DIV_MSB_REG        (0x1)

#define UART_MAX_BAUD           1152000


struct ns16550_record {
    int ioport;
    int reg_shift;
    ulong mmio_base;
};


/*
 * IO helpers
 */
static inline u8 ns16550_io_read(struct ns16550_record *record, int reg_sel)
{
    int ioport = record->ioport;
    ulong addr = record->mmio_base + ((ulong)reg_sel << record->reg_shift);

    if (ioport) {
        return port_read8(addr);
    } else {
        return mmio_read8(addr);
    }
}

static inline void ns16550_io_write(struct ns16550_record *record, int reg_sel, u8 val)
{
    int ioport = record->ioport;
    ulong addr = record->mmio_base + ((ulong)reg_sel << record->reg_shift);

    if (ioport) {
        port_write8(addr, val);
    } else {
        mmio_write8(addr, val);
    }
}


/*
 * putchar
 */
static struct ns16550_record *putchar_record = NULL;

static int ns16550_putchar(int ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = ns16550_io_read(putchar_record, UART_LINE_STAT_REG) & 0x20;
    }

    ns16550_io_write(putchar_record, UART_DATA_REG, (u8)ch & 0xff);
    return 1;
}


/*
 * Driver interface
 */
static void setup(struct driver_param *param)
{
    struct ns16550_record *record = param->record;
    putchar_record = record;
}

static void create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct ns16550_record *record = mempool_alloc(sizeof(struct ns16550_record));
    memzero(record, sizeof(struct ns16550_record));
    param->record = record;

    u64 reg = 0, size = 0;
    int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
    panic_if(next, "ns16550 only supports one reg field!");

    paddr_t mmio_paddr = cast_u64_to_paddr(reg);
    ulong mmio_size = cast_paddr_to_vaddr(size);
    ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);
    record->mmio_base = mmio_vaddr;

    int reg_shift = devtree_get_reg_shift(fw_info->devtree_node);
    int ioport = devtree_get_use_ioport(fw_info->devtree_node);
    record->reg_shift = reg_shift;
    record->ioport = ioport;

    kprintf("Found NS16550 @ %lx, window @ %lx\n", mmio_paddr, mmio_vaddr);
}

static const char *ns16550_devtree_names[] = {
    "ns16550",
    "ns16550a",
    NULL
};

DECLARE_SERIAL_DRIVER(ns16550_uart, "NS16550", ns16550_devtree_names,
                      create, setup, ns16550_putchar);
