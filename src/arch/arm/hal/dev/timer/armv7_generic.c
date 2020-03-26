#include "common/include/inttypes.h"
#include "hal/include/dev.h"
#include "hal/include/kprintf.h"


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
};


static u32 timer_step = 0;


int is_generic_timer_asserted()
{
    struct generic_timer_phys_ctrl_reg ctrl;
    //read_generic_timer_phys_ctrl(ctrl.value);

    return ctrl.asserted;
}

void start_generic_timer()
{
    struct generic_timer_phys_ctrl_reg ctrl;
    ctrl.value = 0;
    ctrl.enabled = 1;

    //write_generic_timer_phys_interval(timer_step);
    //write_generic_timer_phys_ctrl(ctrl.value);
}

void init_generic_timer()
{
    u32 freq = 0;
    //read_generic_timer_freq(freq);
    //assert(freq);

    // 10 times per second
    timer_step = freq / 100;

    // Register the handler
    //set_int_vector(INT_VECTOR_LOCAL_TIMER, int_handler_local_timer);

    //kprintf("Timer freq @ %uHz, step set @ %u\n", freq, timer_step);
}

void init_generic_timer_mp()
{
    u32 freq = 0;
    //read_generic_timer_freq(freq);
    //assert(freq);

    // 10 times per second
    timer_step = freq / 100;

    //kprintf("Timer freq @ %uHz, step set @ %u\n", freq, timer_step);
}

static int probe(struct fw_dev_info *fw_info, void **param)
{
    if (fw_info->devtree_node &&
        match_devtree_compatible(fw_info->devtree_node, "arm,armv7-timer")
    ) {
        kprintf("Found ARMv7 timer!\n");
        return FW_DEV_PROBE_OK;
    }

    return FW_DEV_PROBE_FAILED;
}


DECLARE_DEV_DRIVER(armv7_generic_timer) = {
    .name = "ARMv7 Generic Timer",
    .probe = probe,
};
