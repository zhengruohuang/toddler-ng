#include "common/include/inttypes.h"
#include "hal/include/devtree.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


static int generic_intc_int_handler(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    struct driver_param *param = ictxt->param;
    struct internal_dev_driver *driver = param->driver;

    int handle_type = INT_HANDLE_CALL_KERNEL;

    // Get pending IRQ
    int irq_seq = driver->intc.find_pending_irq(param, ictxt);

    // Handle
    if (irq_seq >= 0 && irq_seq < driver->intc.max_num_devs) {
        struct driver_param *int_dev = driver->intc.devs[irq_seq];
        driver->intc.disable_irq(param, ictxt, irq_seq);

        // HAL handler
        if (int_dev && int_dev->int_seq) {
            ictxt->param = int_dev;
            handle_type = invoke_int_handler(int_dev->int_seq, ictxt, kdi);

            if ((INT_HANDLE_KEEP_MASKED & ~handle_type) &&
                driver->intc.end_irq
            ) {
                driver->intc.end_irq(param, ictxt, irq_seq);
            }
        }

        // User handler
        else if (int_dev) {
            // TODO: need a better way
            kdi->param0 = int_dev->user_seq;
            handle_type = INT_HANDLE_CALL_KERNEL;
        }
    }

    return handle_type;
}

static inline int _generic_irq_raw_to_seq(struct internal_dev_driver *driver,
                                          struct driver_param *param,
                                          void *encode, int allow_invalid)
{
    int irq_seq = driver->intc.irq_raw_to_seq ?
                        driver->intc.irq_raw_to_seq(param, encode) :
                        swap_big_endian32(((int *)encode)[0]);

    panic_if(irq_seq < 0 && !allow_invalid, "Invalid IRQ seq: %d, encode: %d\n",
             irq_seq, swap_big_endian32(((int *)encode)[0]));
    panic_if(irq_seq >= driver->intc.max_num_devs, "IRQ seq %d exceeding limit %d\n",
             irq_seq, irq_seq >= driver->intc.max_num_devs);

    return irq_seq;
}

void generic_intc_dev_eoi(struct driver_param *param,
                          struct driver_int_encode *encode, struct driver_param *dev)
{
    struct internal_dev_driver *driver = param->driver;
    if (!driver->intc.end_irq) {
        return;
    }

    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g += param->intc.num_int_cells) {
        int irq_seq = _generic_irq_raw_to_seq(driver, param, &int_srcs[g], 0);
        driver->intc.end_irq(param, NULL, irq_seq);
    }
}

void generic_intc_dev_setup(struct driver_param *param)
{
    struct internal_dev_driver *driver = param->driver;

    // Interrupt
    if (driver->intc.int_seq >= INT_SEQ_ALLOC_START) {
        param->int_seq = alloc_int_seq(generic_intc_int_handler);
    } else if (driver->intc.int_seq) {
        set_int_handler(driver->intc.int_seq, generic_intc_int_handler);
        param->int_seq = driver->intc.int_seq;
    }

    // Per-CPU
    if (driver->intc.cpu_power_on) {
        REG_SPECIAL_DRV_FUNC(cpu_power_on, param, driver->intc.cpu_power_on);
    }

    // Start secondary CPUs
    if (driver->intc.start_cpu) {
        REG_SPECIAL_DRV_FUNC(start_cpu, param, driver->intc.start_cpu);
    }

    // Setup
    if (driver->intc.setup) {
        driver->intc.setup(param);
    }
}

void generic_intc_dev_setup_int(struct driver_param *param,
                                struct driver_int_encode *encode, struct driver_param *dev)
{
    kprintf("set int, data @ %p, size: %d\n", encode->data, encode->size);

    if (!encode->size || !encode->data) {
        return;
    }

    struct internal_dev_driver *driver = param->driver;
    int num_ints = encode->size / sizeof(int);
    int *int_srcs = encode->data;

    for (int g = 0; g < num_ints; g += param->intc.num_int_cells) {
        int irq_seq = _generic_irq_raw_to_seq(driver, param, &int_srcs[g], 1);
        if (irq_seq < 0) {
            kprintf("Skip invalid IRQ: %d\n", irq_seq);
            continue;
        }

        kprintf("to enable irq seq: %d\n", irq_seq);
        driver->intc.devs[irq_seq] = dev;
        driver->intc.setup_irq(param, irq_seq);
    }
}

void *generic_intc_dev_create(struct fw_dev_info *fw_info, struct driver_param *param)
{
    struct internal_dev_driver *driver = param->driver;
    panic_if(!driver->intc.setup_irq, "intc driver must implement setup_irq!\n");
    panic_if(!driver->intc.enable_irq, "intc driver must implement setup_irq!\n");
    panic_if(!driver->intc.disable_irq, "intc driver must implement setup_irq!\n");

    param->intc.num_int_cells = devtree_get_num_int_cells(fw_info->devtree_node);
    return driver->intc.create ?
                driver->intc.create(fw_info, param) :
                NULL;
}
