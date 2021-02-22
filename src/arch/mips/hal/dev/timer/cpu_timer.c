#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/vecnum.h"
#include "hal/include/kprintf.h"
#include "hal/include/devtree.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define INT_FREQ_DIV    10
#define MS_PER_INT      (1000 / (INT_FREQ_DIV))


struct mips_cpu_timer_record {
    ulong freq;
    u32 timer_step;
    int clock_idx;
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

    // Clock
    //kprintf("clock idx: %d\n", record->clock_idx);
    if (!ictxt->mp_seq && record->clock_idx) {
        clock_advance_ms(record->clock_idx, MS_PER_INT);
    }

    // Next round
    update_compare(record);

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
    struct mips_cpu_timer_record *record = param->record;
    update_compare(record);

    kprintf("MP Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

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
    record->timer_step = record->freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %luHz, step set @ %u, record @ %p\n", record->timer_step, record);

    // Register special function
    REG_SPECIAL_DRV_FUNC(cpu_power_on, param, cpu_power_on);

    // Register clock source
    record->clock_idx = clock_source_register(CLOCK_QUALITY_PERIODIC_LOW_RES);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "mips,mips-cpu-timer",
        "mti,cpu-timer",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct mips_cpu_timer_record *record = mempool_alloc(sizeof(struct mips_cpu_timer_record));
        memzero(record, sizeof(struct mips_cpu_timer_record));

        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        record->freq = devtree_get_clock_frequency(fw_info->devtree_node);

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
