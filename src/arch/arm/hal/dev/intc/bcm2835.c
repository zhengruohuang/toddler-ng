#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
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

struct bcm2835_mmio {
    volatile u32 irq_basic_pending;
    volatile u32 irq_pending1;
    volatile u32 irq_pending2;
    volatile u32 fiq_control;
    volatile u32 enable_irqs1;
    volatile u32 enable_irqs2;
    volatile u32 enable_basic_irqs;
    volatile u32 disable_irqs1;
    volatile u32 disable_irqs2;
    volatile u32 disable_basic_irqs;
} packed_struct;

struct bcm2835_record {
    struct bcm2835_mmio *mmio;
    u32 enabled_mask1, enabled_mask2;
};


/*
 * Driver interface
 */
static void setup(void *param)
{
    struct bcm2835_record *record = param;

    // Disable all
    record->mmio->disable_basic_irqs = 0xffffffff;
    record->mmio->disable_irqs1 = 0xffffffff;
    record->mmio->disable_irqs2 = 0xffffffff;
}

static int probe(struct fw_dev_info *fw_info, void **param)
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
        if (param) {
            *param = record;
        }

        u64 reg = 0, size = 0;
        int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
        panic_if(next, "bcm2835-armctrl-ic only supports one reg field!");

        ulong mmio_paddr = (ulong)reg;
        ulong mmio_vaddr = get_dev_access_window(mmio_paddr, (ulong)size, DEV_PFN_UNCACHED);

        record->mmio = (void *)mmio_vaddr;
        record->enabled_mask1 = record->enabled_mask2 = 0;

        kprintf("Found BCM2835 top-level intc @ %lx, window @ %lx\n",
                mmio_paddr, mmio_vaddr);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(bcm2835_armctrl_intc) = {
    .name = "BCM2835 Top-Level Interrupt Controller",
    .probe = probe,
    .setup = setup,
};
