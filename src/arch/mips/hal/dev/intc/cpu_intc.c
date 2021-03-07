#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define MAX_NUM_INT_SRCS 8

struct mips_cpu_intc_record {
    ulong valid_bitmap;
    int last_seq;
};


/*
 * Int manipulation
 */
static void enable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.im |= 0x1ul << irq_seq;
    write_cp0_status(sr.value);
}

static void disable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.im &= ~(0x1ul << irq_seq);
    write_cp0_status(sr.value);
}

static void end_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    enable_irq(param, ictxt, irq_seq);
}

static void setup_irq(struct driver_param *param, int irq_seq)
{
    struct mips_cpu_intc_record *record = param->record;
    record->valid_bitmap |= 0x1ul << irq_seq;

    enable_irq(param, NULL, irq_seq);
}

static void disable_irq_all(struct driver_param *param)
{
    struct cp0_status sr;
    read_cp0_status(sr.value);
    sr.im = 0;
    write_cp0_status(sr.value);
}


/*
 * Driver interface
 */
static int pending_irq(struct driver_param *param, struct int_context *ictxt)
{
    struct mips_cpu_intc_record *record = param->record;

    struct cp0_cause cause;
    read_cp0_cause(cause.value);

    int seq = record->last_seq + 1;
    if (seq >= MAX_NUM_INT_SRCS) {
        seq = 0;
    }

    ulong bitmap = cause.ip & record->valid_bitmap;
    for (int i = 0; i < MAX_NUM_INT_SRCS; i++) {
        if (bitmap & (0x1ul << seq)) {
            return seq;
        }

        if (++seq >= MAX_NUM_INT_SRCS) {
            seq = 0;
        }
    }

    return -1;
}

static void setup(struct driver_param *param)
{
    disable_irq_all(param);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct mips_cpu_intc_record *record = mempool_alloc(sizeof(struct mips_cpu_intc_record));
    memzero(record, sizeof(struct mips_cpu_intc_record));

    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 1, "#int-cells must be 1\n");

    kprintf("Found MIPS CP0 top-level intc\n");
    return record;
}

static const char *mips_cpu_intc_devtree_names[] = {
    "mti,cpu-interrupt-controller",
    "mips,cpu-cp0-cause",
    NULL
};

DECLARE_INTC_DRIVER(mips_cpu_intc, "MIPS CP0 Interrupt Controller",
                    mips_cpu_intc_devtree_names,
                    create, setup, /*start*/NULL,
                    /*start_cpu*/NULL, /*cpu_power_on*/NULL, /*raw_to_seq*/NULL,
                    setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
