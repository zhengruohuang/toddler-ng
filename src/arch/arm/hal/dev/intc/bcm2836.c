#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


/*
 * Ref
 * https://www.kernel.org/doc/Documentation/devicetree/bindings/interrupt-controller/brcm%2Cbcm2836-l1-intc.txt
 */


struct bcm2836_mmio {
    // 0x00
    volatile u32 control;

    // 0x04
    volatile u32 reserved;

    // 0x08
    volatile u32 core_timer_prescaler;

    // 0x0c
    volatile u32 gpu_int_routing;

    // 0x10
    volatile u32 perf_mon_int_routing_set;
    volatile u32 perf_mon_int_routing_clear;

    // 0x18
    volatile u32 reserved2;

    // 0x1c
    volatile u32 core_timer_lo;
    volatile u32 core_timer_hi;

    // 0x24
    volatile u32 local_int_routing1;
    volatile u32 local_int_routing2;

    // 0x2c
    volatile u32 axi_outstanding_counters;
    volatile u32 axi_outstanding_irq;

    // 0x34
    volatile u32 local_timer_control;
    volatile u32 local_timer_write_flags;

    // 0x3c
    volatile u32 reserved3;

    // 0x40
    volatile u32 core0_timer_int_ctrl;
    volatile u32 core1_timer_int_ctrl;
    volatile u32 core2_timer_int_ctrl;
    volatile u32 core3_timer_int_ctrl;

    // 0x50
    volatile u32 core0_mailbox_int_ctrl;
    volatile u32 core1_mailbox_int_ctrl;
    volatile u32 core2_mailbox_int_ctrl;
    volatile u32 core3_mailbox_int_ctrl;

    // 0x60
    volatile u32 core0_irq_src;
    volatile u32 core1_irq_src;
    volatile u32 core2_irq_src;
    volatile u32 core3_irq_src;

    // 0x70
    volatile u32 core0_fiq_src;
    volatile u32 core1_fiq_src;
    volatile u32 core2_fiq_src;
    volatile u32 core3_fiq_src;

    // 0x80
    volatile u32 core0_mailbox_write_set[4];
    volatile u32 core1_mailbox_write_set[4];
    volatile u32 core2_mailbox_write_set[4];
    volatile u32 core3_mailbox_write_set[4];

    // 0xc0
    volatile u32 core0_mailbox_read_write_high_to_clear[4];
    volatile u32 core1_mailbox_read_write_high_to_clear[4];
    volatile u32 core2_mailbox_read_write_high_to_clear[4];
    volatile u32 core3_mailbox_read_write_high_to_clear[4];
} packed_struct;

struct bcm2836_record {
    struct bcm2836_mmio *mmio;
};


/*
 * Driver interface
 */
static void setup(void *param)
{
    struct bcm2836_record *record = param;

    // Init the four core timer ints - FIQ phys timer non secure
    record->mmio->core0_timer_int_ctrl = 0x20;
    record->mmio->core1_timer_int_ctrl = 0x20;
    record->mmio->core2_timer_int_ctrl = 0x20;
    record->mmio->core3_timer_int_ctrl = 0x20;

    // Init FIQ timer for 4 cores
    record->mmio->core0_fiq_src = 0x2;
    record->mmio->core1_fiq_src = 0x2;
    record->mmio->core2_fiq_src = 0x2;
    record->mmio->core3_fiq_src = 0x2;
}

static int probe(struct fw_dev_info *fw_info, void **param)
{
    static const char *devtree_names[] = {
        "brcm,bcm2836-l1-intc",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct bcm2836_record *record = mempool_alloc(sizeof(struct bcm2836_record));
        memzero(record, sizeof(struct bcm2836_record));
        if (param) {
            *param = record;
        }

        u64 reg = 0, size = 0;
        int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
        panic_if(next, "brcm,bcm2836-l1-intc only supports one reg field!");

        ulong mmio_paddr = (ulong)reg;
        ulong mmio_vaddr = get_dev_access_window(mmio_paddr, (ulong)size, DEV_PFN_UNCACHED);

        record->mmio = (void *)mmio_vaddr;

        kprintf("Found BCM2836 per-CPU intc @ %lx, window @ %lx\n",
                mmio_paddr, mmio_vaddr);
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(bcm2836_percpu_intc) = {
    .name = "BCM2836 Per-CPU Interrupt Controller",
    .probe = probe,
    .setup = setup,
};
