#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


#define INT_FREQ_DIV    8
#define MS_PER_INT      (1000 / (INT_FREQ_DIV))

struct armv7_generic_timer_record {
    u32 timer_step;
};


/*
 * Debug
 */
int is_armv7_generic_timer_asserted()
{
    struct generic_timer_phys_ctrl_reg ctrl;
    read_generic_timer_phys_ctrl(ctrl.value);

    return ctrl.asserted;
}


/*
 * Manipulation
 */
static inline void _calc_timer_interval(struct armv7_generic_timer_record *record)
{
    u32 freq = 0;
    read_generic_timer_freq(freq);
    panic_if(!freq, "Timer doesn't report a valid frequency!\n");

    // Set up the actual freq
    record->timer_step = freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %uHz, step set @ %u, record @ %p\n", freq, record->timer_step, record);
}

static inline void _update_timer_interval(struct armv7_generic_timer_record *record)
{
    write_generic_timer_phys_interval(record->timer_step);
}

static inline void _enable_timer(const char *prefix,
                                 struct armv7_generic_timer_record *record)
{
    // Disable
    struct generic_timer_phys_ctrl_reg ctrl;
    ctrl.value = 0;
    write_generic_timer_phys_ctrl(ctrl.value);

    // Update interval
    _update_timer_interval(record);

    // Enable
    ctrl.enabled = 1;
    write_generic_timer_phys_ctrl(ctrl.value);

    kprintf("%s started! record @ %p, step: %d, ctrl: %x\n",
            prefix, record, record->timer_step, ctrl.value);
}

static inline void _disable_timer_int(struct armv7_generic_timer_record *record)
{
    struct generic_timer_phys_ctrl_reg ctrl;
    ctrl.value = 0;
    ctrl.masked = 1;
    write_generic_timer_phys_ctrl(ctrl.value);
}

static inline void _enable_timer_int(struct armv7_generic_timer_record *record)
{
    struct generic_timer_phys_ctrl_reg ctrl;
    ctrl.value = 0;
    ctrl.enabled = 1;
    write_generic_timer_phys_ctrl(ctrl.value);
}


/*
 * Interrupt
 */
static int armv7_generic_timer_int_handler(struct driver_param *param, int mp_seq, ulong mp_id)
{
    struct armv7_generic_timer_record *record = param->record;

    // Mask the interrupt
    _disable_timer_int(record);

    // Set a new value for compare
    _update_timer_interval(record);

    // Re-enable the interrupt
    _enable_timer_int(record);

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
    struct armv7_generic_timer_record *record = param->record;
    _enable_timer("MP Timer", record);
}

static void start(struct driver_param *param)
{
    struct armv7_generic_timer_record *record = param->record;
    _enable_timer("Timer", record);
}

static void setup(struct driver_param *param)
{
    struct armv7_generic_timer_record *record = param->record;
    _calc_timer_interval(record);
}

static void *create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct armv7_generic_timer_record *record = mempool_alloc(sizeof(struct armv7_generic_timer_record));
    memzero(record, sizeof(struct armv7_generic_timer_record));

    kprintf("Found ARMv7 timer!\n");
    return record;
}

static const char *armv7_generic_timer_devtree_names[] = {
    "arm,armv7-timer",
    NULL
};

DECLARE_TIMER_DRIVER(armv7_generic_timer, "ARMv7 Generic Timer",
                     armv7_generic_timer_devtree_names,
                     create, setup, start, cpu_power_on,
                     CLOCK_QUALITY_PERIODIC_LOW_RES, MS_PER_INT,
                     INT_SEQ_ALLOC_START, armv7_generic_timer_int_handler);
