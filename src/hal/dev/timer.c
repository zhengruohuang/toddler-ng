#include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


static int generic_timer_int_handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    struct driver_param *param = ictxt->param;
    struct internal_dev_driver *driver = param->driver;

    // Clock
    if (!ictxt->mp_seq && driver->timer.clock_idx) {
        clock_advance_ms(driver->timer.clock_idx, driver->timer.clock_period);
    }

    // Handle
    int handle_type = INT_HANDLE_CALL_KERNEL;
    if (driver->timer.int_handler) {
        driver->timer.int_handler(param, ictxt->mp_seq, ictxt->mp_id);
    }

    return handle_type;
}

void generic_timer_dev_setup(struct driver_param *param)
{
    struct internal_dev_driver *driver = param->driver;

    // Interrupt
    if (driver->timer.int_seq >= INT_SEQ_ALLOC_START) {
        param->int_seq = alloc_int_seq(generic_timer_int_handler);
    } else if (driver->timer.int_seq) {
        set_int_handler(driver->timer.int_seq, generic_timer_int_handler);
        param->int_seq = driver->timer.int_seq;
    }

    // Per-CPU
    if (driver->timer.cpu_power_on) {
        REG_SPECIAL_DRV_FUNC(cpu_power_on, param, driver->timer.cpu_power_on);
    }

    // Clock source
    if (driver->timer.clock_type != CLOCK_QUALITY_NO_CLOCK) {
        driver->timer.clock_idx = clock_source_register(driver->timer.clock_type);
    }

    // Setup
    if (driver->timer.setup) {
        driver->timer.setup(param);
    }
}
