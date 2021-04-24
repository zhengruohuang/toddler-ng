#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/cpuid.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define MAX_NUM_INT_SRCS 8

struct io_apic_record {
    ulong valid_bitmap;
    int last_seq;
    ulong mmio_base;
};


/*
 * IO helpers
 */
static inline u32 io_apic_io_read(struct io_apic_record *record, int reg)
{
    volatile u32 *regs = (volatile u32 *)record->mmio_base;
    regs[0] = reg & 0xff;
    u32 val = regs[1];
    return val;
}

static inline void io_apic_io_write(struct io_apic_record *record, int reg, u32 val)
{
    volatile u32 *regs = (volatile u32 *)record->mmio_base;
    regs[0] = reg & 0xff;
    regs[1] = val;
}


/*
 * Int manipulation
 */
static void enable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
//     struct cp0_status sr;
//     read_cp0_status(sr.value);
//     sr.im |= 0x1ul << irq_seq;
//     write_cp0_status(sr.value);
}

static void disable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
//     struct cp0_status sr;
//     read_cp0_status(sr.value);
//     sr.im &= ~(0x1ul << irq_seq);
//     write_cp0_status(sr.value);
}

static void end_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    enable_irq(param, ictxt, irq_seq);
}

static void setup_irq(struct driver_param *param, int irq_seq)
{
    struct io_apic_record *record = param->record;
    record->valid_bitmap |= 0x1ul << irq_seq;

    enable_irq(param, NULL, irq_seq);
}

static void disable_irq_all(struct driver_param *param)
{
//     struct cp0_status sr;
//     read_cp0_status(sr.value);
//     sr.im = 0;
//     write_cp0_status(sr.value);
}


/*
 * Driver interface
 */
static int pending_irq(struct driver_param *param, struct int_context *ictxt)
{
    return 0;
//     struct io_apic_record *record = param->record;
//
//     struct cp0_cause cause;
//     read_cp0_cause(cause.value);
//
//     int seq = record->last_seq + 1;
//     if (seq >= MAX_NUM_INT_SRCS) {
//         seq = 0;
//     }
//
//     ulong bitmap = cause.ip & record->valid_bitmap;
//     if (!bitmap) {
//         return -1;
//     }
//
//     int irq_seq = ctz(bitmap);
//     return irq_seq;
}

static void setup(struct driver_param *param)
{
    disable_irq_all(param);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct io_apic_record *record = mempool_alloc(sizeof(struct io_apic_record));
    memzero(record, sizeof(struct io_apic_record));

    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 1, "#int-cells must be 1\n");

    int has_apic = read_cpuid_apic();
    panic_if(!has_apic, "APIC not supported by CPU!\n");

    struct apic_base_reg abr;
    read_apic_base(abr.value);
    panic_if(!abr.is_bootstrap, "CPU must be bootstrap processor!\n");
    panic_if(!abr.xapic_enabled, "APIC not enabled!\n");

    paddr_t mmio_paddr = 0xfec00000;
    ulong mmio_size = 0x16;
    ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);
    record->mmio_base = mmio_vaddr;

    kprintf("Found IO APIC intc @ %llx, window @ %lx\n", (u64)mmio_paddr, mmio_vaddr);
    return record;
}

static const char *io_apic_devtree_names[] = {
    "intel,io-apic",
    "io-apic",
    NULL
};

DECLARE_INTC_DRIVER(io_apic, "IO APIC Interrupt Controller",
                    io_apic_devtree_names,
                    create, setup, /*setup_cpu_local*/NULL, /*start*/NULL,
                    /*start_cpu*/NULL, /*cpu_power_on*/NULL, /*raw_to_seq*/NULL,
                    setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
