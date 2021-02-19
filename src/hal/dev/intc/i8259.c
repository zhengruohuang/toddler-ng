#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "common/include/io.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


/*
 * Reference
 * https://wiki.osdev.org/8259_PIC
 */
#define ICW1_ICW4       0x01    // ICW4 (not) needed
#define ICW1_SINGLE     0x02    // Single (cascade) mode
#define ICW1_INTERVAL4  0x04    // Call address interval 4 (8)
#define ICW1_LEVEL      0x08    // Level triggered (edge) mode
#define ICW1_INIT       0x10    // Initialization - required!

#define ICW4_8086       0x01    // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO       0x02    // Auto (normal) EOI
#define ICW4_BUF_SLAVE  0x08    // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C    // Buffered mode/master
#define ICW4_SFNM       0x10    // Special fully nested (not)

#define I8259_MAX_CHIPS 8

struct i8259_record {
    int num_chips;
    int ioport;                     // Use io ports on x86
    ulong io[I8259_MAX_CHIPS][2];   // io[chip_sel][data_sel]
    struct driver_param *int_devs[I8259_MAX_CHIPS * 8];
};


/*
 * IO helpers
 */
static inline u8 i8259_io_read(struct i8259_record *record, int chip_sel, int data_sel)
{
    int ioport = record->ioport;
    ulong addr = record->io[chip_sel][data_sel];

    if (ioport) {
        return ioport_read8(addr);
    } else {
        return mmio_read8(addr);
    }
}

static inline void i8259_io_write(struct i8259_record *record, int chip_sel, int data_sel, u8 val)
{
    int ioport = record->ioport;
    ulong addr = record->io[chip_sel][data_sel];

    if (ioport) {
        ioport_write8(addr, val);
    } else {
        mmio_write8(addr, val);
    }
}


/*
 * Int manipulation
 */
static inline int get_chip(int irq, int *idx_in_chip)
{
    if (idx_in_chip) {
        *idx_in_chip = irq & 0x7;
    }

    return irq >> 3;
}

static void enable_irq(struct i8259_record *record, int irq)
{
}

static void disable_irq(struct i8259_record *record, int irq)
{
}

static void eoi_irq(struct i8259_record *record, int irq)
{
}

static void disable_all(struct i8259_record *record)
{

}


/*
 * Interrupt handler
 */
static int invoke(struct i8259_record *record, int irq, struct driver_param *int_dev,
                  struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    if (irq == -1) {
        return INT_HANDLE_SIMPLE;
    }

    disable_irq(record, irq);

    //kprintf("int_dev @ %p, seq: %d\n", int_dev, int_dev->int_seq);

    int handle_type = INT_HANDLE_SIMPLE;
    if (int_dev && int_dev->int_seq) {
        ictxt->param = int_dev->record;
        handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);

        if (INT_HANDLE_KEEP_MASKED & ~handle_type) {
            eoi_irq(record, irq);
        }
    } else if (int_dev) {
        // TODO: need a better way
        kdi->param0 = int_dev->user_seq;
        handle_type = INT_HANDLE_CALL_KERNEL | INT_HANDLE_KEEP_MASKED;
    }

    return handle_type & INT_HANDLE_CALL_KERNEL ?
            INT_HANDLE_CALL_KERNEL : INT_HANDLE_SIMPLE;
}

static int handle(struct i8259_record *record, int irq,
                  struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    int handle_type = INT_HANDLE_SIMPLE;

    return handle_type;
}

static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
//     struct i8259_record *record = ictxt->param;
//
//     struct bcm2835_irqs_basic_pending basic_pending;
//     basic_pending.value = record->mmio->pending_basic.value;

    int handle_type = INT_HANDLE_SIMPLE;

//     //kprintf("bcm2835, value: %x, ap: %x, pend1: %x, pend2: %x\n",
//     //        basic_pending.value, basic_pending.arm_periphs, record->mmio->pending1, record->mmio->pending2);
//
//     if (basic_pending.arm_periphs) {
//         handle_type |= handle(ictxt, kdi, record, 0, basic_pending.arm_periphs);
//     }
//
//     if (basic_pending.more1) {
//         u32 mask = record->mmio->pending1;
//         handle_type |= handle(ictxt, kdi, record, 1, mask);
//     }
//
//     if (basic_pending.more2) {
//         u32 mask = record->mmio->pending2;
//         handle_type |= handle(ictxt, kdi, record, 2, mask);
//     }

    return handle_type;
}


/*
 * EOI
 */
static void eoi(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    struct i8259_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g++) {
        int irq = swap_big_endian32(int_srcs[g]);
        enable_irq(record, irq);
    }
}


/*
 * Driver interface
 */
static void start(struct driver_param *param)
{

}

static void setup_int(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    kprintf("set int, data: %p, size: %d\n", encode->data, encode->size);

    if (!encode->size || !encode->data) {
        return;
    }

    //kprintf("set int, data: %p, size: %d\n", encode->data, encode->size);

    struct i8259_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g++) {
        int irq = swap_big_endian32(int_srcs[g]);

        kprintf("to enable irq: %d\n", irq);

        enable_irq(record, irq);
        record->int_devs[irq] = dev;
    }
}

static void setup(struct driver_param *param)
{
    struct i8259_record *record = param->record;
    disable_all(record);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "intel,i8259",
        "intel,i8259a",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct i8259_record *record = mempool_alloc(sizeof(struct i8259_record));
        memzero(record, sizeof(struct i8259_record));
        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
        panic_if(num_int_cells != 1, "#int-cells must be 1\n");

        int ioport = devtree_get_use_ioport(fw_info->devtree_node);
        int reg_shift = devtree_get_reg_shift(fw_info->devtree_node);
        record->ioport = ioport;

        ulong iobase_lo = -0x1ul;
        ulong iobase_hi = 0;
        int next = 0;
        do {
            u64 reg = 0, size = 0;
            next = devtree_get_translated_reg(fw_info->devtree_node, next, &reg, &size);

            ulong iobase = 0;
            if (ioport) {
                iobase = cast_u64_to_vaddr(reg);
            } else {
                paddr_t mmio_paddr = cast_u64_to_paddr(reg);
                ulong mmio_size = cast_u64_to_vaddr(size);
                iobase = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);
            }

            record->io[record->num_chips][0] = iobase;
            record->io[record->num_chips][1] = iobase + (0x1ul << reg_shift);
            record->num_chips++;

            if (iobase < iobase_lo) iobase_lo = iobase;
            if (iobase > iobase_hi) iobase_hi = iobase;
        } while (next);

        kprintf("Intel 8259 intc @ %lx - %lx\n", iobase_lo, iobase_hi);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(i8259_intc) = {
    .name = "Intel 8259 Interrupt Controller",
    .probe = probe,
    .setup = setup,
    .setup_int = setup_int,
    .start = start,
    .eoi = eoi,
};
