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


struct clint_timer_record {
    ulong freq;
    u32 timer_step;
};


/*
 * Update compare
 */
static void update_compare(struct clint_timer_record *record)
{
//     u32 count = 0;
//     read_cp0_count(count);
//
//     count += record->timer_step;
//     write_cp0_compare(count);
}



/*
 * Interrupt
 */
static int clint_timer_int_handler(struct driver_param *param, struct int_context *ictxt)
{
    struct clint_timer_record *record = param->record;
    update_compare(record);

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
    struct clint_timer_record *record = param->record;
    update_compare(record);

    kprintf("MP Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void start(struct driver_param *param)
{
    struct clint_timer_record *record = param->record;
    update_compare(record);

    kprintf("Timer started! record @ %p, step: %d\n", record, record->timer_step);
}

static void setup(struct driver_param *param)
{
    struct clint_timer_record *record = param->record;

    // Set up the actual freq
    record->timer_step = record->freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %luHz, step set @ %u, record @ %p\n", record->timer_step, record);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct clint_timer_record *record = mempool_alloc(sizeof(struct clint_timer_record));
    memzero(record, sizeof(struct clint_timer_record));

    record->freq = devtree_get_clock_frequency(fw_info->devtree_node);

    kprintf("Found RISC-V clint timer!\n");
    return record;
}

static const char *clint_timer_devtree_names[] = {
    "riscv,clint0",
    NULL
};

DECLARE_TIMER_DRIVER(clint_timer, "RISC-V Clint Timer",
                     clint_timer_devtree_names,
                     create, setup, start, cpu_power_on,
                     CLOCK_QUALITY_PERIODIC_LOW_RES, MS_PER_INT,
                     INT_SEQ_ALLOC_START, clint_timer_int_handler);
