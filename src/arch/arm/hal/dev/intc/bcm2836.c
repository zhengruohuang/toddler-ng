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
    struct driver_param *int_devs[10];
};


/*
 * Int manipulation
 */
static void enable_irq(struct bcm2836_record *record, int mp_seq, int irq)
{
    panic_if(irq < 0 || irq > 9, "Invalid IRQ!\n");
    panic_if(mp_seq < 0 || mp_seq > 3, "Invalid MP seq!\n");

    if (irq < 4) {
        record->mmio->core_timer_int_ctrl[mp_seq].value |= 0x1 << irq;
    } else if (irq < 8) {
        record->mmio->core_mailbox_int_ctrl[mp_seq].value |= 0x1 << (irq - 4);
    } else if (irq == 8) {
        record->mmio->gpu_int_routing.value = 0;
        record->mmio->local_int_routing.value = 0;
    } else if (irq == 9) {
        record->mmio->pmu_int_routing_set.value = 0x1 << mp_seq;
    }
}

static void disable_irq(struct bcm2836_record *record, int mp_seq, int irq)
{
    panic_if(irq < 0 || irq > 9, "Invalid IRQ!\n");
    panic_if(mp_seq < 0 || mp_seq > 3, "Invalid MP seq!\n");

    if (irq < 4) {
        record->mmio->core_timer_int_ctrl[mp_seq].value &= ~(0x1 << irq);
    } else if (irq < 8) {
        record->mmio->core_mailbox_int_ctrl[mp_seq].value &= ~(0x1 << (irq - 4));
    } else if (irq == 8) {
        // Cannot be disabled
    } else if (irq == 9) {
        record->mmio->pmu_int_routing_clear.value = 0x1 << mp_seq;
    }
}


/*
 * Interrupt handler
 */
static int invoke(struct bcm2836_record *record, int cpu,
                  int irq, struct driver_param *int_dev,
                  struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    if (irq == -1) {
        return INT_HANDLE_SIMPLE;
    }

    disable_irq(record, cpu, irq);

    int handle_type = INT_HANDLE_SIMPLE;
    if (int_dev && int_dev->int_seq) {
        ictxt->param = int_dev;
        handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);

        if (INT_HANDLE_KEEP_MASKED & ~handle_type) {
            enable_irq(record, cpu, irq);
        }
    }

    return handle_type & INT_HANDLE_CALL_KERNEL ?
            INT_HANDLE_CALL_KERNEL : INT_HANDLE_SIMPLE;
}

static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    struct driver_param *param = ictxt->param;
    struct bcm2836_record *record = param->record;
    int cpu = ictxt->mp_seq;

    u32 irq_mask = record->mmio->core_irq_src[cpu].value;
    int num_irqs = popcount32(irq_mask);
    int handle_type = INT_HANDLE_SIMPLE;

    while (num_irqs) {
        int irq = ctz32(irq_mask);
        struct driver_param *int_dev = record->int_devs[irq];

        handle_type |= invoke(record, cpu, irq, int_dev, ictxt, kdi);

        irq_mask &= ~(0x1 << irq);
        num_irqs = popcount32(irq_mask);
    }

    return handle_type;
}


/*
 * Driver interface
 */
static void start_cpu(struct driver_param *param, int seq, ulong id, ulong entry)
{
    panic_if(seq >= 4, "Unable to start CPU #%ld @ %lx\n", seq, id);

    int cur_seq = get_cur_mp_seq();
    panic_if(seq == cur_seq, "Unable to start myself CPU #%ld @ %lx\n", seq, id);

    kprintf("Seq: %d, ID: %lx, entry @ %lx\n", seq, id, entry);

    struct bcm2836_record *record = param->record;
    record->mmio->core_mailbox_write_set[seq].mb3 = entry;
}

static void start(struct driver_param *param)
{
}

static void setup_int(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev)
{
    struct bcm2836_record *record = param->record;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g += 2) {
        int irq = swap_big_endian32(int_srcs[g]);

        switch (irq) {
        case 1:
            record->int_devs[1] = dev;
            for (int c = 0; c < 4; c++) {
                enable_irq(record, c, irq);
            }
            break;
        case 8:
            record->int_devs[8] = dev;
            enable_irq(record, 0, irq);
            break;
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 9:
            break;
        default:
            panic("Unknown int src: %d\n", irq);
            break;
        }
    }
}

static void setup(struct driver_param *param)
{
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

        int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
        panic_if(num_int_cells != 2, "#int-cells must be 2\n");

        u64 reg = 0, size = 0;
        int next = devtree_get_translated_reg(fw_info->devtree_node, 0, &reg, &size);
        panic_if(next, "brcm,bcm2836-l1-intc only supports one reg field!");

        paddr_t mmio_paddr = cast_u64_to_paddr(reg);
        ulong mmio_size = cast_paddr_to_vaddr(size);
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
