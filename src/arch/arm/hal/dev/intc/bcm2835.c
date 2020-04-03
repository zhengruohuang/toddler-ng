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

    u32 enable1;
    u32 enable2;
    u32 enable_basic;

    u32 disable1;
    u32 disable2;
    u32 disable_basic;
} packed4_struct;

struct bcm2835_record {
    volatile struct bcm2835_mmio *mmio;
    struct driver_param *int_devs[72];
};


/*
 * Debug
 */
#define BCM2835_BASE            0x3f000000ul
#define BCM2835_PL011_BASE      (0x201000ul)

struct bcm2835_pl011 {
    u32 DR;            // Data Register
    u32 RSRECR;        // Receive status register/error clear register
    u32 PAD[4];        // Padding
    u32 FR;            // Flag register
    u32 RES1;          // Reserved
    u32 ILPR;          // Not in use
    u32 IBRD;          // Integer Baud rate divisor
    u32 FBRD;          // Fractional Baud rate divisor
    u32 LCRH;          // Line Control register
    u32 CR;            // Control register
    u32 IFLS;          // Interupt FIFO Level Select Register
    u32 IMSC;          // Interupt Mask Set Clear Register
    u32 RIS;           // Raw Interupt Status Register
    u32 MIS;           // Masked Interupt Status Register
    u32 ICR;           // Interupt Clear Register
    u32 DMACR;         // DMA Control Register
} packed4_struct;

void enable_pl011()
{
    ulong paddr = BCM2835_BASE + BCM2835_PL011_BASE;
    hal_map_range(paddr, paddr, PAGE_SIZE, 0);

    volatile struct bcm2835_pl011 *pl011 = (void *)paddr;

    // Clear the receiving FIFO
    while (!(pl011->FR & 0x10)) {
        (void)pl011->DR;
    }

    // Register the int handler
    //int_ctrl_register(57, bcm2835_pl011_int_handler);

    // Enable receiving interrupt and disable irrevelent ones
    pl011->IMSC = 0x10;

    // Clear all interrupt status
    pl011->ICR = 0x7FF;
}


/*
 * Int manipulation
 */
static int irq_to_dev(int bank, int irq)
{
    switch (bank) {
    case 0:
        panic_if(irq > 7, "Unknown IRQ: %d\n", irq);
        return irq;
    case 1:
        panic_if(irq < 0 || irq > 31, "Unknown IRQ: %d\n", irq);
        return irq + 8;
    case 2:
        panic_if(irq < 0 || irq > 31, "Unknown IRQ: %d\n", irq);
        return irq + 8 + 32;
    default:
        panic("Unknown IRQ: %d\n", irq);
        return -1;
    }

    return -1;
}

static void enable_irq(struct bcm2835_record *record, int bank, int irq)
{
    switch (bank) {
    case 0:
        panic_if(irq > 7, "Unknown IRQ: %d\n", irq);
        record->mmio->enable_basic = 0x1 << irq;
        break;
    case 1:
        panic_if(irq < 0 || irq > 31, "Unknown IRQ: %d\n", irq);
        record->mmio->enable1 = 0x1 << irq;
        break;
    case 2:
        panic_if(irq < 0 || irq > 31, "Unknown IRQ: %d\n", irq);
        record->mmio->enable2 = 0x1 << irq;
        break;
    default:
        panic("Unknown IRQ: %d\n", irq);
    }
}

static void disable_irq(struct bcm2835_record *record, int bank, int irq)
{
    switch (bank) {
    case 0:
        panic_if(irq > 7, "Unknown IRQ: %d\n", irq);
        record->mmio->disable_basic = 0x1 << irq;
        break;
    case 1:
        panic_if(irq < 0 || irq > 31, "Unknown IRQ: %d\n", irq);
        record->mmio->disable1 = 0x1 << irq;
        break;
    case 2:
        panic_if(irq < 0 || irq > 31, "Unknown IRQ: %d\n", irq);
        record->mmio->disable2 = 0x1 << irq;
        break;
    default:
        panic("Unknown IRQ: %d\n", irq);
    }
}

static void disable_all(struct bcm2835_record *record)
{
    record->mmio->disable_basic = 0xffffffff;
    record->mmio->disable1 = 0xffffffff;
    record->mmio->disable2 = 0xffffffff;
}


/*
 * Interrupt handler
 */
static int handle_irq(struct int_context *ictxt, struct kernel_dispatch_info *kdi,
                      struct bcm2835_record *record, int *handle_type,
                      int bank, u32 mask, int irq_count,
                      volatile u32 *enable, volatile u32 *disable)
{
    int num_irqs = popcount32(mask);

    // Only one
    if (num_irqs == 1) {
        int irq = ctz32(mask);
        int dev = irq_to_dev(bank, irq);
        struct driver_param *int_dev = record->int_devs[dev];

        if (int_dev && int_dev->int_seq) {
            *handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);
            return 1;
        }
    }

    // More than one
    else if (num_irqs > 1) {
        for (int i = 0; i < irq_count; i++) {
            u32 irq = 0x1 << i;
            int dev = irq_to_dev(bank, irq);
            struct driver_param *int_dev = record->int_devs[dev];

            if (mask & irq && int_dev && int_dev->int_seq) {
                *handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);
                return 1;
            }
        }
    }

    return 0;
}

static int handler(struct int_context *ictxt, struct kernel_dispatch_info *kdi)
{
    struct bcm2835_record *record = ictxt->param;
    int cpu = ictxt->mp_seq;

    struct bcm2835_irqs_basic_pending basic_pending;
    basic_pending.value = record->mmio->pending_basic.value;

    int handled = 0;
    int handle_type = INT_HANDLE_TYPE_HAL;

    if (basic_pending.arm_periphs) {
        handled = handle_irq(ictxt, kdi, record, &handle_type,
                             0, basic_pending.arm_periphs, 8,
                             &record->mmio->enable_basic,
                             &record->mmio->disable_basic);
    }

    if (!handled && basic_pending.more1) {
        handled = handle_irq(ictxt, kdi, record, &handle_type,
                             1, record->mmio->pending1, 32,
                             &record->mmio->enable1,
                             &record->mmio->disable1);
    }

    if (!handled && basic_pending.more2) {
        handled = handle_irq(ictxt, kdi, record, &handle_type,
                             1, record->mmio->pending2, 32,
                             &record->mmio->enable2,
                             &record->mmio->disable2);
    }

    return handle_type;
}


/*
 * Driver interface
 */
static void start(struct driver_param *param)
{
    //struct bcm2835_record *record = param;
    //enable_wired(record, 57);
}

static void setup_int(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    if (!encode->size || !encode->data) {
        return;
    }

    kprintf("set int, data: %p, size: %d\n", encode->data, encode->size);

    struct bcm2835_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g += 2) {
        int bank = swap_big_endian32(int_srcs[g]);
        int irq = swap_big_endian32(int_srcs[g + 1]);
        int seq = irq_to_dev(bank, irq);

        enable_irq(record, bank, irq);
        record->int_devs[seq] = dev;
    }
}

static void setup(struct driver_param *param)
{
    struct bcm2835_record *record = param->record;
    disable_all(record);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "brcm,bcm2835-armctrl-ic",
        "brcm,bcm2836-armctrl-ic",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct bcm2835_record *record = mempool_alloc(sizeof(struct bcm2835_record));
        memzero(record, sizeof(struct bcm2835_record));
        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        u64 reg = 0, size = 0;
        int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
        panic_if(next, "bcm2835-armctrl-ic only supports one reg field!");

        ulong mmio_paddr = (ulong)reg;
        ulong mmio_size = (ulong)size;
        ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);

        record->mmio = (void *)mmio_vaddr;

        kprintf("Found BCM2835 top-level intc @ %lx, window @ %lx, size: %ld\n",
                mmio_paddr, mmio_vaddr, mmio_size);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(bcm2835_armctrl_intc) = {
    .name = "BCM2835 Top-Level Interrupt Controller",
    .probe = probe,
    .setup = setup,
    .setup_int = setup_int,
    .start = start,
};
