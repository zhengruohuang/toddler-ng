#include "common/include/inttypes.h"
#include "common/include/msr.h"
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

struct armv7_generic_timer_record {
    u32 timer_step;
};


/*
 * Debug
 */
int is_generic_timer_asserted()
{
    struct generic_timer_phys_ctrl_reg ctrl;
    read_generic_timer_phys_ctrl(ctrl.value);

    return ctrl.asserted;
}


/*
 * Interrupt
 */
static int handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    //kprintf("Timer!\n");

    struct armv7_generic_timer_record *record = ictxt->param;
    struct generic_timer_phys_ctrl_reg ctrl;

    // Mask the interrupt
    ctrl.value = 0;
    ctrl.enabled = 0;
    ctrl.masked = 1;
    write_generic_timer_phys_ctrl(ctrl.value);

    // Set a new value for compare
    write_generic_timer_phys_interval(record->timer_step);

    // Re-enable the interrupt
    ctrl.enabled = 1;
    ctrl.masked = 0;
    write_generic_timer_phys_ctrl(ctrl.value);

    return INT_HANDLE_CALL_KERNEL;
}


/*
 * Driver interface
 */
static void cpu_power_on(struct driver_param *param, int seq, ulong id)
{
    struct armv7_generic_timer_record *record = param->record;

    struct generic_timer_phys_ctrl_reg ctrl;
    ctrl.value = 0;
    write_generic_timer_phys_ctrl(ctrl.value);

    ctrl.enabled = 1;
    write_generic_timer_phys_interval(record->timer_step);
    write_generic_timer_phys_ctrl(ctrl.value);

    kprintf("MP Timer started! record @ %p, step: %d, ctrl: %x\n", record, record->timer_step, ctrl.value);
}

static void start(struct driver_param *param)
{
    struct armv7_generic_timer_record *record = param->record;

    struct generic_timer_phys_ctrl_reg ctrl;
    ctrl.value = 0;
    write_generic_timer_phys_ctrl(ctrl.value);

    ctrl.enabled = 1;
    write_generic_timer_phys_interval(record->timer_step);
    write_generic_timer_phys_ctrl(ctrl.value);

    kprintf("Timer started! record @ %p, step: %d, ctrl: %x\n", record, record->timer_step, ctrl.value);
}

static void setup(struct driver_param *param)
{
    struct armv7_generic_timer_record *record = param->record;

    u32 freq = 0;
    read_generic_timer_freq(freq);
    panic_if(!freq, "Timer doesn't report a valid frequency!\n");

    // Set up the actual freq
    record->timer_step = freq / INT_FREQ_DIV;
    kprintf("Timer freq @ %uHz, step set @ %u, record @ %p\n", freq, record->timer_step, record);

    // Register special function
    REG_SPECIAL_DRV_FUNC(cpu_power_on, param, cpu_power_on);
}

static int probe(struct fw_dev_info *fw_info, struct driver_param *param)
{
    static const char *devtree_names[] = {
        "arm,armv7-timer",
        NULL
    };

    if (fw_info->devtree_node &&
        match_devtree_compatibles(fw_info->devtree_node, devtree_names)
    ) {
        struct armv7_generic_timer_record *record = mempool_alloc(sizeof(struct armv7_generic_timer_record));
        memzero(record, sizeof(struct armv7_generic_timer_record));
        param->record = record;
        param->int_seq = alloc_int_seq(handler);

        kprintf("Found ARMv7 timer!\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}

DECLARE_DEV_DRIVER(armv7_generic_timer) = {
    .name = "ARMv7 Generic Timer",
    .probe = probe,
    .setup = setup,
    .start = start,
};
