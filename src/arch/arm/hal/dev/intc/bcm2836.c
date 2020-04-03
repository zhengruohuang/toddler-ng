#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


/*
 * Ref
 * https://www.kernel.org/doc/Documentation/devicetree/bindings/interrupt-controller/brcm%2Cbcm2836-l1-intc.txt
 */


struct bcm2836_mmio {
    // 0x00
    struct {
        union {
            u32 value;
            struct {
                u32 reserved0           : 8;
                u32 core_timer_clk_src  : 1;    // 0 = Crystal, 1 = APB
                u32 core_timer_inc_step : 1;    // 0 = Inc1, 1 = Inc2
                u32 reserved1           : 22;
            };
        };
    } control;

    // 0x04
    u32 reserved;

    // 0x08
    struct {
        u32 value;
    } core_timer_prescaler;

    // 0x0c
    struct {
        union {
            u32 value;
            struct {
                u32 irq         : 2;    // CPU index
                u32 fiq         : 2;
                u32 reserved    : 28;
            };
        };
    } gpu_int_routing;

    // 0x10
    struct {
        union {
            u32 value;
            struct {
                u32 irq0        : 1;    // 1 = Enabled, ignored if the
                u32 irq1        : 1;    // corresponding FIQ bit is set
                u32 irq2        : 1;
                u32 irq3        : 1;
                u32 fiq0        : 1;
                u32 fiq1        : 1;
                u32 fiq2        : 1;
                u32 fiq3        : 1;
                u32 reserved    : 24;
            };
        };
    } pmu_int_routing_set, pmu_int_routing_clear;

    // 0x18
    u32 reserved2;

    // 0x1c
    struct {
        union {
            u64 value;
            struct {
                u32 lo;
                u32 hi;
            };
        };
    } core_timer;

    // 0x24
    struct {
        union {
            u32 value;
            struct {
                u32 cpu         : 2;    // CPU index
                u32 fiq         : 1;    // Go to FIQ instead of IRQ
                u32 reserved    : 29;
            };
        };
    } local_int_routing;

    // 0x28
    u32 reserved3;

    // 0x2c
    struct {
        union {
            u32 value;
            struct {
                u32 reads   : 10;
                u32 zero1   : 6;
                u32 writes  : 10;
                u32 zero2   : 6;
            };
        };
    } axi_outstanding_counters;

    // 0x30
    struct {
        union {
            u32 value;
            struct {
                u32 timeout_hi24    : 20;
                u32 int_enabled     : 1;
                u32 reserved        : 11;
            };
        };
    } axi_outstanding_irq;

    // 0x34
    struct {
        union {
            u32 value;
            struct {
                u32 reload      : 28;
                u32 enabled     : 1;
                u32 int_enabled : 1;
                u32 reserved    : 1;
                u32 int_flag    : 1;
            };
        };
    } local_timer_ctrl;

    // 0x38
    struct {
        union {
            u32 value;
            struct {
                u32 reserved    : 30;
                u32 reloaded    : 1;
                u32 clear_flag  : 1;
            };
        };
    } local_timer_ctrl_write;

    // 0x3c
    u32 reserved4;

    // 0x40
    // Indexed by CPU index
    struct {
        union {
            u32 value;
            struct {
                u32 irq_ps      : 1;    // 1 = Enabled, ignored if the
                u32 irq_pns     : 1;    // corresponding FIQ bit is set
                u32 irq_hp      : 1;
                u32 irq_v       : 1;
                u32 fiq_ps      : 1;
                u32 fiq_pns     : 1;
                u32 fiq_hp      : 1;
                u32 fiq_v       : 1;
                u32 reserved    : 24;
            };
        };
    } core_timer_int_ctrl[4];

    // 0x50
    // Indexed by CPU index
    struct {
        union {
            u32 value;
            struct {
                u32 irq0        : 1;    // 1 = Enabled, ignored if the
                u32 irq1        : 1;    // corresponding FIQ bit is set
                u32 irq2        : 1;
                u32 irq3        : 1;
                u32 fiq0        : 1;    // 1 = Enabled, ignored if the
                u32 fiq1        : 1;    // corresponding FIQ bit is set
                u32 fiq2        : 1;
                u32 fiq3        : 1;
                u32 reserved    : 24;
            };
        };
    } core_mailbox_int_ctrl[4];

    // 0x60
    // Indexed by CPU index
    struct {
        union {
            u32 value;
            struct {
                u32 timer_ps    : 1;
                u32 timer_pns   : 1;
                u32 timer_hp    : 1;
                u32 timer_v     : 1;
                u32 mb0         : 1;
                u32 mb1         : 1;
                u32 mb2         : 1;
                u32 mb3         : 1;
                u32 gpu         : 1;    // Can be high in one core only
                u32 pmu         : 1;
                u32 axi         : 1;    // For CPU0 only
                u32 local_timer : 1;
                u32 periphs     : 16;   // Unused
                u32 reserved    : 4;
            };
        };
    } core_irq_src[4];

    // 0x70
    // Indexed by CPU index
    struct {
        union {
            u32 value;
            struct {
                u32 timer_ps    : 1;
                u32 timer_pns   : 1;
                u32 timer_hp    : 1;
                u32 timer_v     : 1;
                u32 mb0         : 1;
                u32 mb1         : 1;
                u32 mb2         : 1;
                u32 mb3         : 1;
                u32 gpu         : 1;    // Can be high in one core only
                u32 pmu         : 1;
                u32 zero        : 1;
                u32 local_timer : 1;
                u32 periphs     : 16;   // Unused
                u32 reserved    : 4;
            };
        };
    } core_fiq_src[4];

    // 0x80
    // Indexed by CPU index
    struct {
        u32 mb0;
        u32 mb1;
        u32 mb2;
        u32 mb3;
    } core_mailbox_write_set[4];

    // 0xc0
    // Indexed by CPU index
    struct {
        u32 mb0;
        u32 mb1;
        u32 mb2;
        u32 mb3;
    } core_mailbox_read_write_high_to_clear[4];
} packed4_struct;

struct bcm2836_record {
    volatile struct bcm2836_mmio *mmio;
    struct driver_param *int_devs[8];
};


/*
 * Interrupt handler
 */
static int handler(struct int_context *ictxt, struct kernel_dispatch_info *kdi)
{
    struct bcm2836_record *record = ictxt->param;
    int cpu = ictxt->mp_seq;

    u32 irq_mask = record->mmio->core_irq_src[cpu].value;
    int num_irqs = popcount32(irq_mask);

    int handle_type = INT_HANDLE_TYPE_HAL;

    // Only one
    if (num_irqs == 1) {
        int irq = ctz32(irq_mask);
        struct driver_param *int_dev = record->int_devs[irq];
        if (int_dev && int_dev->int_seq) {
            handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);
        }
    }

    // More than one
    else if (num_irqs > 1) {
        for (int i = 0; i < 8; i++) {
            u32 irq = 0x1 << i;
            struct driver_param *int_dev = record->int_devs[irq];
            if (irq_mask & irq && int_dev && int_dev->int_seq) {
                handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);
                break;
            }
        }
    }

    return handle_type;
}


/*
 * Driver interface
 */
static void start_cpu(void *param, int seq, ulong id, ulong entry)
{
    panic_if(seq >= 4, "Unable to start CPU #%ld @ %lx\n", seq, id);

    int cur_seq = get_cur_mp_seq();
    panic_if(seq == cur_seq, "Unable to start myself CPU #%ld @ %lx\n", seq, id);

    kprintf("Seq: %d, ID: %lx, entry @ %lx\n", seq, id, entry);

    struct bcm2836_record *record = param;
    record->mmio->core_mailbox_write_set[seq].mb3 = entry;
}

static void start(struct driver_param *param)
{

}

static void setup_int(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    kprintf("set int, data: %p, size: %d\n", encode->data, encode->size);

    struct bcm2836_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g += 2) {
        int src = swap_big_endian32(int_srcs[g]);

        switch (src) {
        case 0:
        case 2:
        case 3:
            break;
        case 1:
            for (int c = 0; c < 4; c++) {
                record->mmio->core_timer_int_ctrl[c].value = 0x2;
            }
            record->int_devs[1] = dev;
            break;
        case 8:
        case 9:
            break;
        default:
            panic("Unknown int src: %d\n", src);
            break;
        }
    }
}

static void setup(struct driver_param *param)
{
    struct bcm2836_record *record = param->record;

    // Init the four core timer ints - IRQ phys timer non secure
//     record->mmio->core_timer_int_ctrl[0].value = 0x2;
//     record->mmio->core_timer_int_ctrl[1].value = 0x2;
//     record->mmio->core_timer_int_ctrl[2].value = 0x2;
//     record->mmio->core_timer_int_ctrl[3].value = 0x2;

    // Register special function
    REG_SPECIAL_DRV_FUNC(start_cpu, param, start_cpu);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
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
        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        u64 reg = 0, size = 0;
        int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
        panic_if(next, "brcm,bcm2836-l1-intc only supports one reg field!");

        ulong mmio_paddr = (ulong)reg;
        ulong mmio_size = (ulong)size;
        ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);

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
    .setup_int = setup_int,
    .start = start,
};
