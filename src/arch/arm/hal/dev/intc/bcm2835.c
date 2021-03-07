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
 * Ref
 * https://www.kernel.org/doc/Documentation/devicetree/bindings/interrupt-controller/brcm%2Cbcm2835-armctrl-ic.txt
 */
#define BCM2835_BASIC_ARM_TIMER_IRQ         (0x1 << 0)
#define BCM2835_BASIC_ARM_MAILBOX_IRQ       (0x1 << 1)
#define BCM2835_BASIC_ARM_DOORBELL_0_IRQ    (0x1 << 2)
#define BCM2835_BASIC_ARM_DOORBELL_1_IRQ    (0x1 << 3)
#define BCM2835_BASIC_GPU_0_HALTED_IRQ      (0x1 << 4)
#define BCM2835_BASIC_GPU_1_HALTED_IRQ      (0x1 << 5)
#define BCM2835_BASIC_ACCESS_ERROR_1_IRQ    (0x1 << 6)
#define BCM2835_BASIC_ACCESS_ERROR_0_IRQ    (0x1 << 7)

#define MAX_NUM_INT_SRCS 96

struct bcm2835_irqs_basic_pending {
    union {
        u32 value;
        struct {
            u32 arm_timer       : 1;
            u32 arm_mailbox     : 1;
            u32 arm_doorbell0   : 1;
            u32 arm_doorbell1   : 1;
            u32 gpu_halted0     : 1;
            u32 gpu_halted1     : 1;
            u32 access_err0     : 1;
            u32 access_err1     : 1;
            u32 reserved        : 24;
        };
        struct {
            u32 arm_periphs     : 8;
            u32 more1           : 1;
            u32 more2           : 1;
            u32 reserved2       : 22;
        };
    };
} packed4_struct;

struct bcm2835_mmio {
    struct bcm2835_irqs_basic_pending pending_basic;
    u32 pending1;
    u32 pending2;

    struct {
        union {
            u32 value;
            struct {
                u32 src         : 7;
                u32 enabled     : 1;
                u32 reserved    : 24;
            };
        };
    } fiq_ctrl;

    u32 enable[3];
//     u32 enable1;
//     u32 enable2;
//     u32 enable_basic;

    u32 disable[3];
//     u32 disable1;
//     u32 disable2;
//     u32 disable_basic;
} packed4_struct;

struct bcm2835_record {
    // [0] = basic (bank0), [1] = bank1, [2] = bank2
    u32 valid_bitmap[3];
    volatile struct bcm2835_mmio *mmio;
};


/*
 * Int manipulation
 */
static inline void _enable_irq(struct bcm2835_record *record, int bank, int irq)
{
    int reg_idx = bank == 0 ? 2 : bank - 1;
    record->mmio->enable[reg_idx] = 0x1 << irq;
}

static void _disable_irq(struct bcm2835_record *record, int bank, int irq)
{
    int reg_idx = bank == 0 ? 2 : bank - 1;
    record->mmio->disable[reg_idx] = 0x1 << irq;
}

static void disable_all(struct bcm2835_record *record)
{
    record->mmio->disable[0] = 0xffffffff;
    record->mmio->disable[1] = 0xffffffff;
    record->mmio->disable[2] = 0xffffffff;
}


/*
 * IRQ
 */
static inline int _irq_seq_to_raw(int seq, int *bank_irq)
{
    if (bank_irq) {
        *bank_irq = seq & 0x1f;
    }
    return seq >> 5;
}

static inline int _irq_raw_to_seq(int bank, int bank_irq)
{
    switch (bank) {
    case 0:
        panic_if(bank_irq > 7, "Unknown bank %d IRQ: %d\n", bank, bank_irq);
        break;
    case 1:
    case 2:
        panic_if(bank_irq < 0 || bank_irq > 31,
                 "Unknown bank %d IRQ: %d\n", bank, bank_irq);
        break;
    default:
        panic("Unknown bank %d IRQ: %d\n", bank, bank_irq);
        break;
    }

    int seq = (bank << 5) | bank_irq;
    return seq;
}

static int irq_raw_to_seq(struct driver_param *param, void *encode)
{
    int *pair = encode;
    int bank = swap_big_endian32(pair[0]);
    int bank_irq = swap_big_endian32(pair[1]);

    int seq = _irq_raw_to_seq(bank, bank_irq);
    return seq;
}

static void enable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct bcm2835_record *record = param->record;

    int bank_irq = 0;
    int bank = _irq_seq_to_raw(irq_seq, &bank_irq);
    _enable_irq(record, bank, bank_irq);
}

static void disable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct bcm2835_record *record = param->record;

    int bank_irq = 0;
    int bank = _irq_seq_to_raw(irq_seq, &bank_irq);
    _disable_irq(record, bank, bank_irq);
}

static void end_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    enable_irq(param, ictxt, irq_seq);
}

static void setup_irq(struct driver_param *param, int irq_seq)
{
    struct bcm2835_record *record = param->record;

    int bank_irq = 0;
    int bank = _irq_seq_to_raw(irq_seq, &bank_irq);

    record->valid_bitmap[bank] |= 0x1 << bank_irq;
    _enable_irq(record, bank, bank_irq);
}


/*
 * Driver interface
 */
static int pending_irq(struct driver_param *param, struct int_context *ictxt)
{
    struct bcm2835_record *record = param->record;

    struct bcm2835_irqs_basic_pending basic_pending;
    basic_pending.value = record->mmio->pending_basic.value;

//     kprintf("bcm2835, value: %x, ap: %x, pend1: %x, pend2: %x\n",
//            basic_pending.value, basic_pending.arm_periphs, record->mmio->pending1, record->mmio->pending2);

    int bank = -1;
    u32 bitmap = 0;

    if (basic_pending.arm_periphs) {
        bank = 0;
        bitmap = basic_pending.arm_periphs & record->valid_bitmap[0];
    }

    if (!bitmap && basic_pending.more1) {
        bank = 1;
        bitmap = record->mmio->pending1 & record->valid_bitmap[1];
    }

    if (!bitmap && basic_pending.more2) {
        bank = 2;
        bitmap = record->mmio->pending2 & record->valid_bitmap[2];
    }

    if (!bitmap) {
        return -1;
    }

    int bank_irq = ctz32(bitmap);
    return _irq_raw_to_seq(bank, bank_irq);
}

static void setup(struct driver_param *param)
{
    struct bcm2835_record *record = param->record;
    disable_all(record);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct bcm2835_record *record = mempool_alloc(sizeof(struct bcm2835_record));
    memzero(record, sizeof(struct bcm2835_record));

    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 2, "#int-cells must be 2\n");

    u64 reg = 0, size = 0;
    int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
    panic_if(next, "bcm2835-armctrl-ic only supports one reg field!");

    paddr_t mmio_paddr = cast_u64_to_paddr(reg);
    ulong mmio_size = cast_paddr_to_vaddr(size);
    ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);

    record->mmio = (void *)mmio_vaddr;

    kprintf("Found BCM2835 top-level intc @ %lx, window @ %lx, size: %ld\n",
            mmio_paddr, mmio_vaddr, mmio_size);
    return record;
}

static const char *bcm2835_armctrl_intc_devtree_names[] = {
    "brcm,bcm2835-armctrl-ic",
    "brcm,bcm2836-armctrl-ic",
    NULL
};

DECLARE_INTC_DRIVER(bcm2835_armctrl_intc, "BCM2835 Top-Level Interrupt Controller",
                    bcm2835_armctrl_intc_devtree_names,
                    create, setup, /*start*/NULL,
                    /*start_cpu*/NULL, /*cpu_power_on*/NULL,
                    irq_raw_to_seq, setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
