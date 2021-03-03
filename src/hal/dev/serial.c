#include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


void generic_serial_dev_setup(struct driver_param *param)
{
    struct internal_dev_driver *driver = param->driver;

    if (driver->serial.setup) {
        driver->serial.setup(param);
    }

    panic_if(!driver->serial.putchar, "Serial driver must implement putchar!\n");

    init_libk_putchar(driver->serial.putchar);
    kprintf("Putchar switched to %s driver\n", driver->name);
}
