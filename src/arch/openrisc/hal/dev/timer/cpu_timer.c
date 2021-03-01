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


struct or1k_cpu_timer_record {
    ulong freq;
    u32 timer_step;
    int clock_idx;
};


/*
 * Update compare
 */
static void update_compare(struct or1k_cpu_timer_record *record)
{
    struct tick_mode_reg ttmr;
    read_tick_mode(ttmr.value);
    ttmr.int_pending = 0;
    write_tick_mode(ttmr.value);

    write_tick_count(0);
}

static void init_tick(struct or1k_cpu_timer_record *record)
{
    struct tick_mode_reg ttmr;
    ttmr.value = 0;
    ttmr.mode = 0x3; // Continuous timer
    ttmr.int_enabled = 1;
    ttmr.int_pending = 0;
    ttmr.period = record->timer_step;

    write_tick_mode(ttmr.value);
}



/*
 * Interrupt
 */
static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
//     u32 count = 0;
//     read_tick_count(count);
//     kprintf("Timer count: %u\n", count);

    struct or1k_cpu_timer_record *record = ictxt->param;

    // Clock
    //kprintf("mp seq: %d, clock idx: %d\n", ictxt->mp_seq, record->clock_idx);
    if (!ictxt->mp_seq && 1 /*record->clock_idx*/) {
        clock_advance_ms(1 /*record->clock_idx*/, MS_PER_INT);
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
    struct or1k_cpu_timer_record *record = param->record;
    init_tick(record);
    update_compare(record);

    kprintf("MP Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void start(struct driver_param *param)
{
    struct or1k_cpu_timer_record *record = param->record;
    init_tick(record);
    update_compare(record);

    kprintf("Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void setup(struct driver_param *param)
{
    struct or1k_cpu_timer_record *record = param->record;

    // Set up the actual freq
    record->timer_step = record->freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %luHz, step set @ %u, record @ %p\n", record->timer_step, record);

    // Register special function
    REG_SPECIAL_DRV_FUNC(cpu_power_on, param, cpu_power_on);

    // Register clock source
    record->clock_idx = clock_source_register(CLOCK_QUALITY_PERIODIC_LOW_RES);
    //kprintf("idx: %d\n", record->clock_idx);
    //while (1);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "or1k,or1k-cpu-timer",
        "or1k-cpu-timer",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct or1k_cpu_timer_record *record = mempool_alloc(sizeof(struct or1k_cpu_timer_record));
        memzero(record, sizeof(struct or1k_cpu_timer_record));

        param->record = record;
        //param->int_seq = alloc_int_seq(handler);
        param->int_seq = EXCEPT_NUM_TIMER;
        set_int_handler(EXCEPT_NUM_TIMER, handler);

        record->freq = devtree_get_clock_frequency(fw_info->devtree_node);

        kprintf("Found OR1K CPU timer!\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}

DECLARE_DEV_DRIVER(or1k_cpu_timer) = {
    .name = "OpenCores OR1K CPU Timer",
    .probe = probe,
    .setup = setup,
    .start = start,
};
