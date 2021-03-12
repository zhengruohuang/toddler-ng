#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define MAX_NUM_INT_SRCS (ARCH_WIDTH)

struct riscv_cpu_intc_record {
    ulong valid_bitmap;
};


/*
 * Int manipulation
 */
static void enable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    ulong sie = 0;
    read_sie(sie);
    sie |= 0x1ul << irq_seq;
    write_sie(sie);
}

static void disable_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    ulong sie = 0;
    read_sie(sie);
    sie &= ~(0x1ul << irq_seq);
    write_sie(sie);
}

static void end_irq(struct driver_param *param, struct int_context *ictxt, int irq_seq)
{
    enable_irq(param, ictxt, irq_seq);
}

static void setup_irq(struct driver_param *param, int irq_seq)
{
    // Skip hypervisor ints
    if (irq_seq == 2 || irq_seq == 6) {
        return;
    }

    // Redirect machine ints to supervisor
    if (irq_seq == 3 || irq_seq == 7) {
        irq_seq -= 2;
    }

    struct riscv_cpu_intc_record *record = param->record;
    record->valid_bitmap |= 0x1ul << irq_seq;

    enable_irq(param, NULL, irq_seq);
}

static void disable_irq_all(struct driver_param *param)
{
    write_sie(0);
}


/*
 * Driver interface
 */
static int pending_irq(struct driver_param *param, struct int_context *ictxt)
{
    struct riscv_cpu_intc_record *record = param->record;

    ulong sip = 0;
    read_sip(sip);

    ulong bitmap = sip & record->valid_bitmap;
    return bitmap ? ctz(bitmap) : -1;
}

static void setup(struct driver_param *param)
{
    disable_irq_all(param);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct riscv_cpu_intc_record *record = mempool_alloc(sizeof(struct riscv_cpu_intc_record));
    memzero(record, sizeof(struct riscv_cpu_intc_record));

    int num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    panic_if(num_int_cells != 1, "#int-cells must be 1\n");

    kprintf("Found RISC-V hart-level intc\n");
    return record;
}

static const char *riscv_cpu_intc_devtree_names[] = {
    "riscv,cpu-intc",
    NULL
};

DECLARE_INTC_DRIVER(riscv_cpu_intc, "RISC-V Hart-Level Interrupt Controller",
                    riscv_cpu_intc_devtree_names,
                    create, setup, /*start*/NULL,
                    /*start_cpu*/NULL, /*cpu_power_on*/NULL, /*raw_to_seq*/NULL,
                    setup_irq, enable_irq, disable_irq, end_irq,
                    pending_irq, MAX_NUM_INT_SRCS,
                    INT_SEQ_ALLOC_START);
