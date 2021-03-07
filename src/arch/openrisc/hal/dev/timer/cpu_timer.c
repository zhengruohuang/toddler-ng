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
 * Update timer
 */
static void reset_tick()
{
    struct tick_mode_reg ttmr;
    read_tick_mode(ttmr.value);
    ttmr.int_pending = 0;
    write_tick_mode(ttmr.value);

    write_tick_count(0);
}

static void init_period(struct or1k_cpu_timer_record *record)
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
static int or1k_cpu_timer_int_handler(struct driver_param *param, struct int_context *ictxt)
{
    reset_tick();
    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
    struct or1k_cpu_timer_record *record = param->record;
    init_period(record);
    reset_tick();

    kprintf("MP Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void start(struct driver_param *param)
{
    struct or1k_cpu_timer_record *record = param->record;
    init_period(record);
    reset_tick();

    kprintf("Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void setup(struct driver_param *param)
{
    struct or1k_cpu_timer_record *record = param->record;

    // Set up the actual freq
    record->timer_step = record->freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %luHz, step set @ %u, record @ %p\n", record->timer_step, record);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct or1k_cpu_timer_record *record = mempool_alloc(sizeof(struct or1k_cpu_timer_record));
    memzero(record, sizeof(struct or1k_cpu_timer_record));

    record->freq = devtree_get_clock_frequency(fw_info->devtree_node);

    kprintf("Found OR1K CPU timer!\n");
    return record;
}

static const char *or1k_cpu_timer_devtree_names[] = {
    "or1k,or1k-cpu-timer",
    "or1k-cpu-timer",
    NULL
};

DECLARE_TIMER_DRIVER(or1k_cpu_timer, "OpenCores OR1K CPU Timer",
                     or1k_cpu_timer_devtree_names,
                     create, setup, start, cpu_power_on,
                     CLOCK_QUALITY_PERIODIC_LOW_RES, MS_PER_INT,
                     EXCEPT_NUM_TIMER, or1k_cpu_timer_int_handler);
