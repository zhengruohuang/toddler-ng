#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/vecnum.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define INT_FREQ_DIV    8


struct generic_timer_phys_ctrl_reg {
    union {
        u32 value;

        struct {
            u32 enabled     : 1;
            u32 masked      : 1;
            u32 asserted    : 1;
            u32 reserved    : 29;
        };
    };
} packed4_struct;

struct mips_cpu_timer_record {
    u32 timer_step;
};


/*
 * Update compare
 */
static void update_compare(struct mips_cpu_timer_record *record)
{
    u32 count = 0;
    read_cp0_count(count);

    count += record->timer_step;
    write_cp0_compare(count);
}



/*
 * Interrupt
 */
static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    //kprintf("Timer!\n");

    struct mips_cpu_timer_record *record = ictxt->param;
    update_compare(record);

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void start(struct driver_param *param)
{
    struct mips_cpu_timer_record *record = param->record;
    update_compare(record);

    kprintf("Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void setup(struct driver_param *param)
{
    struct mips_cpu_timer_record *record = param->record;

    // Set up the actual freq
    record->timer_step = 0x2FAF080 / INT_FREQ_DIV;

    kprintf("Timer freq @ %uHz, step set @ %u, record @ %p\n", record->timer_step, record);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "mips,mips-cpu-timer",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct mips_cpu_timer_record *record = mempool_alloc(sizeof(struct mips_cpu_timer_record));
        memzero(record, sizeof(struct mips_cpu_timer_record));

        param->record = record;

        // The built-in timer uses a pre-defined interrupt seq number
        param->int_seq = INT_VECTOR_LOCAL_TIMER;
        set_int_handler(INT_VECTOR_LOCAL_TIMER, handler);

        kprintf("Found MIPS CPU timer!\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}

DECLARE_DEV_DRIVER(mips_cpu_timer) = {
    .name = "MIPS CPU Timer",
    .probe = probe,
    .setup = setup,
    .start = start,
};
