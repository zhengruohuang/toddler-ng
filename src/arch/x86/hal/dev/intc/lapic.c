#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/setup.h"
#include "hal/include/cpuid.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


/*
 * Registers
 */
enum local_apic_regs {
    LAPIC_REG_ID            = 0x20,
    LAPIC_REG_VER           = 0x30,
    LAPIC_REG_TASK_PRIO     = 0x80,
    LAPIC_REG_ARB_PRIO      = 0x90,
    LAPIC_REG_PROC_PRIO     = 0xa0,
    LAPIC_REG_EOI           = 0xb0,
    LAPIC_REG_REMOTE_READ   = 0xc0,
    LAPIC_REG_LOGICAL_DST   = 0xd0,
    LAPIC_REG_DST_FORMAT    = 0xe0,
    LAPIC_REG_SPURIOUS_VEC  = 0xf0,
    LAPIC_REG_ISR           = 0x100,
    LAPIC_REG_TRIGGER_MODE  = 0x180,
    LAPIC_REG_INT_REQ       = 0x200,
    LAPIC_REG_ERR_STATUS    = 0x280,
    LAPIC_REG_LVT_CMCI      = 0x2f0,
    LAPIC_REG_INT_CMD_LOW   = 0x300,
    LAPIC_REG_INT_CMD_HIGH  = 0x310,
    LAPIC_REG_LVT_TIMER     = 0x320,
    LAPIC_REG_LVT_THERMAL   = 0x330,
    LAPIC_REG_LVT_PERF_MON  = 0x340,
    LAPIC_REG_LVT_LINT0     = 0x350,
    LAPIC_REG_LVT_LINT1     = 0x360,
    LAPIC_REG_LVT_ERR       = 0x370,
    LAPIC_REG_INIT_COUNT    = 0x380,
    LAPIC_REG_CUR_COUNT     = 0x390,
    LAPIC_REG_TIMER_DIV     = 0x3e0,
};

struct apic_int_cmd_reg {
    union {
        struct {
            u32     low;
            u32     high;
        } value;

        struct {
            struct {
                u32 vector          : 8;
                u32 deliver_mode    : 3;
                u32 dest_logical    : 1;
                u32 deliver_pending : 1;
                u32 reserved_1      : 1;
                u32 level_assert    : 1;
                u32 level_trigger   : 1;
                u32 reserved_2      : 2;
                u32 dest_shorthand  : 2;
                u32 reserved_3      : 12;
            };

            struct {
                u32 reserved        : 24;
                u32 dest_apic_id    : 8;
            };
        };
    };
} packed4_struct;

struct apic_err_status_reg {
    union {
        u32         value;

        struct {
            u32     send_checksum           : 1;
            u32     receive_checksum        : 1;
            u32     send_accept             : 1;
            u32     receive_accept          : 1;
            u32     reserved1               : 1;
            u32     illegal_vec_send        : 1;
            u32     illegal_vec_recv        : 1;
            u32     illegal_reg_addr        : 1;
            u32     reserved2               : 24;
        };

        struct {
            u32     error_bitmap            : 8;
            u32     reserved3               : 24;
        };
    };
} packed4_struct;

struct apic_lvt_timer_reg {
    union {
        u32         value;
        struct {
            u32     vector          : 8;
            u32     reserved1       : 4;
            u32     deliver_pending : 1;
            u32     reserved2       : 3;
            u32     masked          : 1;
            u32     periodic        : 1;
            u32     reserved3       : 14;
        };
    };
} packed4_struct;

struct apic_spurious_vector_reg {
    union {
        u32         value;
        struct {
            u32     vector          : 8;
            u32     lapic_enabled   : 1;
            u32     focus_checking  : 1;
            u32     reserved        : 22;
        };
    };
} packed4_struct;

struct apic_task_prio_reg {
    union {
        u32 value;
        struct {
            u32     prio_subclass   : 4;
            u32     prio            : 4;
        };
    };
} packed4_struct;


/*
 * Driver record
 */
#define MAX_NUM_INT_SRCS 8

struct local_apic_record {
    ulong valid_bitmap;
    int last_seq;
    ulong mmio_base;
};


/*
 * IO helpers
 */
static inline u32 lapic_io_read(struct local_apic_record *record, int offset)
{
    volatile u32 *reg = (volatile u32 *)(record->mmio_base + offset);
    u32 val = *reg;
    return val;
}

static inline void lapic_io_write(struct local_apic_record *record, int offset, u32 val)
{
    volatile u32 *reg = (volatile u32 *)(record->mmio_base + offset);
    *reg = val;
}


/*
 * APIC ID
 */
static struct local_apic_record *the_record = NULL;

static ulong read_apic_id()
{
    return lapic_io_read(the_record, LAPIC_REG_ID);
}


/*
 * Status check
 */
static int lapic_check_error(struct local_apic_record *record)
{
    struct apic_err_status_reg esr;
    esr.value = lapic_io_read(record, LAPIC_REG_ERR_STATUS);
    return esr.error_bitmap;
}

static void lapic_clear_error(struct local_apic_record *record)
{
    lapic_io_write(record, LAPIC_REG_ERR_STATUS, 0);
}


/*
 * IPI
 */
enum apic_ipi_type {
    APIC_IPI_INIT_ASSERT,
    APIC_IPI_INIT_DEASSERT,
    APIC_IPI_STARTUP,
};

static void lapic_send_ipi(struct local_apic_record *record,
                           int type, u32 dst_apic_id, u32 vec)
{
    struct apic_int_cmd_reg icr;
    icr.value.low = lapic_io_read(record, LAPIC_REG_INT_CMD_LOW);
    icr.value.high = lapic_io_read(record, LAPIC_REG_INT_CMD_HIGH);

    switch (type) {
    case APIC_IPI_INIT_ASSERT:
    case APIC_IPI_INIT_DEASSERT:
        icr.deliver_mode = 0x5;
        icr.level_assert = type == APIC_IPI_INIT_ASSERT ? 1 : 0;
        break;
    case APIC_IPI_STARTUP:
        icr.deliver_mode = 0x6;
        icr.level_assert = 0;
        break;
    default:
        panic("Unknown IPI type: %d\n", type);
        break;
    }

    icr.level_trigger = 0;
    icr.dest_shorthand = 0;
    icr.dest_logical = 0;
    icr.vector = vec;
    icr.dest_apic_id = dst_apic_id;

    lapic_io_write(record, LAPIC_REG_INT_CMD_HIGH, icr.value.high);
    lapic_io_write(record, LAPIC_REG_INT_CMD_LOW, icr.value.low);
}

static int lapic_wait_ipi_delivery(struct local_apic_record *record)
{
    struct apic_int_cmd_reg icr;
    do {
        icr.value.low = lapic_io_read(record, LAPIC_REG_INT_CMD_LOW);
    } while (icr.deliver_pending);

    return 0;
}


/*
 * Timer
 */
static u32 get_cpu_max_mhz()
{
    u32 core_base = 0, core_max = 0, bus = 0;
    u32 tsc = read_tsc_mhz();
    read_cpu_mhz(&core_base, &core_max, &bus);

    kprintf("MHz: Base @ %u, Max @ %u, Bus @ %u, TSC @ %u\n", core_base, core_max, bus, tsc);
    return 1000;
}

static void lapic_block_delay(struct local_apic_record *record, int ms)
{
    u32 mhz = 1000; // FIXME: assume processor runs @ 1GHz
    u32 ticks = mhz * 1000 * ms / 16;
    const u32 init_count = 0xffffffff;

    struct apic_lvt_timer_reg tm = { .value = 0 };
    tm.masked = 1;
    tm.vector = 0;
    lapic_io_write(record, LAPIC_REG_LVT_TIMER, tm.value);

    lapic_io_write(record, LAPIC_REG_TIMER_DIV, 0x3);
    lapic_io_write(record, LAPIC_REG_INIT_COUNT, init_count);

    u32 count0 = lapic_io_read(record, LAPIC_REG_CUR_COUNT), count1 = 0;
    do {
        count1 = lapic_io_read(record, LAPIC_REG_CUR_COUNT);
    } while (count0 == count1);

    u32 elapsed_ticks = 0;
    while (elapsed_ticks < ticks) {
        elapsed_ticks = init_count - lapic_io_read(record, LAPIC_REG_CUR_COUNT);
    }

//     kprintf("elapsed_ticks: %x\n", elapsed_ticks);
//     for (volatile int m = 0; m < ms; m++) {
//         for (volatile int i = 0; i < 1000; i++);
//     }
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
    struct local_apic_record *record = param->record;
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
//     struct local_apic_record *record = param->record;
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

static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
//     struct local_apic_record *record = param->record;
//
//     const int timer_irq = 7;
//     if (record->valid_bitmap & timer_irq) {
//         enable_irq(param, NULL, timer_irq);
//     }
}

static void start_cpu(struct driver_param *param, int seq, ulong id, ulong entry)
{
    get_cpu_max_mhz();

    struct local_apic_record *record = param->record;

    int cur_seq = arch_get_cur_mp_seq();
    ulong cur_id = arch_get_cur_mp_id();
    panic_if(seq == cur_seq || id == cur_id,
             "Unable to start myself CPU #%ld @ %lx\n", seq, id);

    int err = 0;
    kprintf("To start CPU @ seq: %d, ID: %lx, entry @ %lx\n", seq, id, entry);
    lapic_clear_error(record);

    // INIT - assert
    lapic_send_ipi(record, APIC_IPI_INIT_ASSERT, id, 0);
    err = lapic_wait_ipi_delivery(record);
    panic_if(err, "IPI failed!\n");

    // INIT - deassert
    lapic_send_ipi(record, APIC_IPI_INIT_DEASSERT, id, 0);
    err = lapic_wait_ipi_delivery(record);
    panic_if(err, "IPI failed!\n");

    lapic_block_delay(record, 10);

    // STARTUP x2
    for (int i = 0; i < 2; i++) {
        lapic_clear_error(record);

        lapic_send_ipi(record, APIC_IPI_STARTUP, id, entry >> 12);
        lapic_block_delay(record, 1); // 200us
        err = lapic_wait_ipi_delivery(record);
    }

    kprintf("All IPIs sent\n");

//     for (int s = 0; s < 10; s++) {
//         lapic_block_delay(record, 1000);
//         kprintf("Wait %d seconds\n", s + 1);
//     }
//
//     kprintf("here?\n");
}

static void setup(struct driver_param *param)
{
    struct local_apic_record *record = param->record;

    disable_irq_all(param);

    the_record = record;
    setup_mp_id(read_apic_id);

    ulong apic_id = get_cur_mp_id();
    kprintf("Bootstrap MP ID: %lx\n", apic_id);

//     // Task priority
//     struct apic_task_prio_reg tpr;
//     tpr.value = lapic_io_read(record, LAPIC_REG_TASK_PRIO);
//     tpr.prio_subclass = 0;
//     tpr.prio = 0;
//     lapic_io_write(record, LAPIC_REG_TASK_PRIO, tpr.value);
//
//     // Spurious interrupt
//     struct apic_spurious_vector_reg svr;
//     svr.value = lapic_io_read(record, LAPIC_REG_SPURIOUS_VEC);
//     svr.focus_checking = 1;
//     svr.lapic_enabled = 1;
//     svr.vector = 32;
//     lapic_io_write(record, LAPIC_REG_SPURIOUS_VEC, svr.value);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct local_apic_record *record = mempool_alloc(sizeof(struct local_apic_record));
    memzero(record, sizeof(struct local_apic_record));

    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 1, "#int-cells must be 1\n");

    int has_apic = read_cpuid_apic();
    panic_if(!has_apic, "APIC not supported by CPU!\n");

    struct apic_base_reg abr;
    read_apic_base(abr.value);
    panic_if(!abr.is_bootstrap, "CPU must be bootstrap processor!\n");
    panic_if(!abr.xapic_enabled, "APIC not enabled!\n");

    paddr_t mmio_paddr = ppfn_to_paddr(abr.apic_base_pfn);
    ulong mmio_size = 0x400;
    ulong mmio_vaddr = get_dev_access_window(mmio_paddr, mmio_size, DEV_PFN_UNCACHED);
    record->mmio_base = mmio_vaddr;

    kprintf("Found CPU-local APIC intc @ %llx, window @ %lx\n", (u64)mmio_paddr, mmio_vaddr);
    return record;
}

static const char *local_apic_devtree_names[] = {
    "intel,local-apic",
    "local-apic",
    NULL
};

DECLARE_INTC_DRIVER(local_apic, "Local APIC Interrupt Controller",
                    local_apic_devtree_names,
                    create, setup, /*setup_cpu_local*/NULL, /*start*/NULL,
                    start_cpu, cpu_power_on, /*raw_to_seq*/NULL,
                    setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
