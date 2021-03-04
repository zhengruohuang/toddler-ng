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
static int mips_cpu_timer_int_handler(struct driver_param *param, int mp_seq, ulong mp_id)
{
    struct mips_cpu_timer_record *record = param->record;
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
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct mips_cpu_timer_record *record = mempool_alloc(sizeof(struct mips_cpu_timer_record));
    memzero(record, sizeof(struct mips_cpu_timer_record));

    record->freq = devtree_get_clock_frequency(fw_info->devtree_node);

    kprintf("Found MIPS CPU timer!\n");
    return record;
}

static const char *mips_cpu_timer_devtree_names[] = {
    "mips,mips-cpu-timer",
    "mti,cpu-timer",
    NULL
};

DECLARE_TIMER_DRIVER(mips_cpu_timer, "MIPS CPU Timer",
                     mips_cpu_timer_devtree_names,
                     create, setup, start, cpu_power_on,
                     CLOCK_QUALITY_PERIODIC_LOW_RES, MS_PER_INT,
                     INT_SEQ_ALLOC_START, mips_cpu_timer_int_handler);
