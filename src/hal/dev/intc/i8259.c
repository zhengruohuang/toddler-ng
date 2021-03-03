#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
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
#define ICW4_BUF_MASTER 0x0c    // Buffered mode/master
#define ICW4_SFNM       0x10    // Special fully nested (not)

#define OCW2_SET        0x0     // Indicate OCW2 word
#define OCW2_EOI        0x20
#define OCW2_PRIO       0x40
#define OCW2_PRIO_ROT   0x80

#define OCW3_READ_IRR   0x2
#define OCW3_READ_ISR   0x3
#define OCW3_POLL       0x4
#define OCW3_SET        0x8     // Indicate OCW3 word
#define OCW3_SMM        0x20
#define OCW3_ESMM       0x40

#define I8259_MAX_CHIPS 8

struct i8259_record {
    int poll;                       // Issue poll command to obtain ISR
    int num_chips;
    int ioport;                     // Use io ports on x86
    ulong io[I8259_MAX_CHIPS][2];   // io[chip_sel][data_sel]
    u8 mask[I8259_MAX_CHIPS];
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
        return port_read8(addr);
    } else {
        return mmio_read8(addr);
    }
}

static inline void i8259_io_write(struct i8259_record *record, int chip_sel, int data_sel, u8 val)
{
    int ioport = record->ioport;
    ulong addr = record->io[chip_sel][data_sel];

    if (ioport) {
        port_write8(addr, val);
    } else {
        mmio_write8(addr, val);
    }
}


/*
 * Cascade mask
 */
static inline u8 get_cascade_mask(struct i8259_record *record)
{
    if (record->num_chips == 1) {
        return 0;
    } else if (record->num_chips == 7) {
        return 0xfd; /* 8'b11111101 */
    }

    return ((0x1 << (record->num_chips - 1)) - 0x1) << 2;
}

static inline u8 get_cascade_pos_mask(struct i8259_record *record, int chip)
{
    if (!chip) {
        return 0;
    }

    return chip == 7 ? 0x1 : (0x4 << (chip - 1));
}

static inline int get_cascade_chip(struct i8259_record *record, u8 mask)
{
    if (mask & 0x1) {
        return 7;
    }

    return ctz32(mask) - 2 + 1;
}


/*
 * Int manipulation
 */
static inline int get_seq(int chip, int chip_irq)
{
    return chip * 8 + chip_irq;
}

static inline int get_chip_and_irq(int seq, int *chip_irq)
{
    if (chip_irq) {
        *chip_irq = seq & 0x7;
    }

    return seq >> 3;
}

static inline void enable_irq(struct i8259_record *record, int chip, int chip_irq)
{
    u8 mask = i8259_io_read(record, chip, 1);
    mask &= ~(0x1 << chip_irq);
    i8259_io_write(record, chip, 1, mask);

    // Unmask primary chip if needed
    if (chip && mask == 0xff) {
        u8 mask = i8259_io_read(record, 0, 1);
        mask &= ~(chip == 7 ? 0x1 : (0x4 << (chip - 1)));
        i8259_io_write(record, 0, 1, mask);
    }
}

static inline void enable_int(struct i8259_record *record, int seq)
{
    int chip_irq = 0;
    int chip = get_chip_and_irq(seq, &chip_irq);
    enable_irq(record, chip, chip_irq);
}

static inline void disable_irq(struct i8259_record *record, int chip, int chip_irq)
{
    u8 mask = i8259_io_read(record, chip, 1);
    mask |= 0x1 << chip_irq;
    i8259_io_write(record, chip, 1, mask);

    // Mask primary chip if needed
    if (chip && mask == 0xff) {
        u8 mask = i8259_io_read(record, 0, 1);
        mask |= chip == 7 ? 0x1 : (0x4 << (chip - 1));
        i8259_io_write(record, 0, 1, mask);
    }
}

static inline void disable_int(struct i8259_record *record, int seq)
{
    int chip_irq = 0;
    int chip = get_chip_and_irq(seq, &chip_irq);
    disable_irq(record, chip, chip_irq);
}

static inline void end_of_irq(struct i8259_record *record, int chip, int chip_irq)
{
    i8259_io_write(record, chip, 0, OCW2_SET | OCW2_EOI);
    if (chip) {
        i8259_io_write(record, 0, 0, OCW2_SET | OCW2_EOI);
    }
}

static inline void disable_all(struct i8259_record *record)
{
    for (int i = 0; i < record->num_chips; i++) {
        i8259_io_write(record, 0, 1, 0xff);
        record->mask[i] = 0xff;
    }
}

static inline u8 read_isr(struct i8259_record *record, int chip)
{
    if (record->poll) {
        // Enter poll mode, with the following read as INT ack
        i8259_io_write(record, chip, 0, OCW3_SET | OCW3_POLL);
        i8259_io_read(record, chip, 0);
    }

    i8259_io_write(record, chip, 0, OCW3_SET | OCW3_READ_ISR);
    u8 isr = i8259_io_read(record, chip, 0);
    //kprintf("ISR (%d): %x\n", chip, isr);

    if (record->poll) {
        // Leave poll mode
        i8259_io_write(record, chip, 0, OCW3_SET);
        i8259_io_read(record, chip, 0);
    }

    return isr;
}

static inline void init_i8259(struct i8259_record *record)
{
    // ICW1 - Init
    u8 icw1 = ICW1_INIT | ICW1_ICW4;
    if (record->num_chips == 1)
        icw1 |= ICW1_SINGLE;
    for (int i = 0; i < record->num_chips; i++) {
        i8259_io_write(record, i, 0, icw1);
    }

    // ICW2 - Int vector
    for (int i = 0; i < record->num_chips; i++) {
        i8259_io_write(record, i, 1, 0x8 * i);
    }

    // ICW3 - Secondary chips
    if (record->num_chips > 1) {
        u8 primary_icw3 = get_cascade_mask(record);
        i8259_io_write(record, 0, 1, primary_icw3);
        for (int i = 1; i < record->num_chips; i++) {
            i8259_io_write(record, i, 1, 0x1 << i);
        }
    }

    // ICW4
    for (int i = 0; i < record->num_chips; i++) {
        i8259_io_write(record, i, 1, ICW4_8086);
    }

//     i8259_io_write(record, 0, 0, 0x11);     // Master 8259, ICW1
//     i8259_io_write(record, 1, 0, 0x11);     // Slave  8259, ICW1
//     i8259_io_write(record, 0, 1, 0x0);      // Master 8259, ICW2, set the initial vector of Master 8259 to 0x0.
//     i8259_io_write(record, 1, 1, 0x8);      // Slave  8259, ICW2, set the initial vector of Slave 8259 to 0x8.
//     i8259_io_write(record, 0, 1, 0x4);      // Master 8259, ICW3, IR2 to Slave 8259
//     i8259_io_write(record, 1, 1, 0x2);      // Slave  8259, ICW3, the counterpart to IR2 or Master 8259
//     i8259_io_write(record, 0, 1, 0x1);      // Master 8259, ICW4
//     i8259_io_write(record, 1, 1, 0x1);      // Slave  8259, ICW4
}


/*
 * Interrupt handler
 */
static inline int invoke(struct i8259_record *record, int chip, int chip_irq,
                         struct driver_param *int_dev,
                         struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    disable_irq(record, chip, chip_irq);

    //kprintf("int_dev @ %p, seq: %d\n", int_dev, int_dev->int_seq);

    int handle_type = INT_HANDLE_SIMPLE;
    if (int_dev && int_dev->int_seq) {
        ictxt->param = int_dev->record;
        handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);
    } else if (int_dev) {
        // TODO: need a better way
        kdi->param0 = int_dev->user_seq;
        handle_type = INT_HANDLE_CALL_KERNEL | INT_HANDLE_KEEP_MASKED;
    }

    end_of_irq(record, chip, chip_irq);
    if (INT_HANDLE_KEEP_MASKED & ~handle_type) {
        enable_irq(record, chip, chip_irq);
    }

    return handle_type & INT_HANDLE_CALL_KERNEL ?
            INT_HANDLE_CALL_KERNEL : INT_HANDLE_SIMPLE;
}

static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    struct i8259_record *record = ictxt->param;

    int chip = 0;
    u8 cascade_mask = get_cascade_mask(record);

    // Check primary chip
    u8 isr = read_isr(record, 0);
    u8 secondary_mask = isr & cascade_mask;

    // Int from primary
    if (secondary_mask) {
        chip = get_cascade_chip(record, secondary_mask);
        isr = read_isr(record, chip);
    }

    int chip_irq = ctz32((u32)isr);
    int seq = get_seq(chip, chip_irq);
    //kprintf("i8259, chip: %d, chip irq: %d, seq: %d\n", chip, chip_irq, seq);

    struct driver_param *int_dev = record->int_devs[seq];
    int handle_type = invoke(record, chip, chip_irq, int_dev, ictxt, kdi);

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
        int seq = swap_big_endian32(int_srcs[g]);
        //kprintf("EOI: %d\n", seq);
        enable_int(record, seq);
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

        enable_int(record, irq);
        record->int_devs[irq] = dev;
    }
}

static void setup(struct driver_param *param)
{
    struct i8259_record *record = param->record;

    init_i8259(record);
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

        int reg_shift = devtree_get_reg_shift(fw_info->devtree_node);
        int ioport = devtree_get_use_ioport(fw_info->devtree_node);
        record->ioport = ioport;
        record->poll = devtree_get_use_poll(fw_info->devtree_node);

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
