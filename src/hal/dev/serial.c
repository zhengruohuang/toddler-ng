#include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


void generic_serial_dev_setup(struct driver_param *param)
{
    struct internal_dev_driver *driver = param->driver;
    panic_if(!driver->serial.putchar, "Serial driver must implement putchar!\n");

    if (driver->serial.setup) {
        driver->serial.setup(param);
    }
}


void generic_serial_dev_start(struct driver_param *param)
{
    struct internal_dev_driver *driver = param->driver;

    init_libk_putchar(driver->serial.putchar);
    kprintf("Putchar switched to %s driver\n", driver->name);
}
