#ifndef __HAL_INCLUDE_DRIVER__
#define __HAL_INCLUDE_DRIVER__


#include "common/include/inttypes.h"


/*
 * Device driver
 */
struct fw_dev_info {
    void *devtree_node;
};

struct driver_param {
    struct internal_dev_driver *driver;

    void *record;
    int int_seq;    // HAL interrupt sequence
    int user_seq;   // User-space handler sequence
};

struct driver_int_encode {
    int size;
    void *data;
};

struct internal_dev_driver {
    const char *name;
    struct internal_dev_driver *next;

    const char **probe_devtree_compatible;
    int (*probe)(struct fw_dev_info *fw_info, struct driver_param *param);

    void *(*create)(struct fw_dev_info *fw_info, struct driver_param *param);
    void (*setup)(struct driver_param *param);
    void (*start)(struct driver_param *param);

    void (*setup_int)(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev);
    void (*eoi)(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev);

    union {
        struct {
            void (*setup)(struct driver_param *param);
            int (*putchar)(int ch);
        } serial;

        struct {
            void (*setup)(struct driver_param *param);
            void (*cpu_power_on)(struct driver_param *param, int mp_seq, ulong mp_id);

            int clock_type;
            int clock_period;
            int clock_idx;

            int int_seq;
            int (*int_handler)(struct driver_param *param, int mp_seq, ulong mp_id);
        } timer;
    };
};

extern struct internal_dev_driver *get_all_internal_drivers();
extern void register_dev_driver(struct internal_dev_driver *drv);


/*
 * Generic driver interfaces
 */
extern void generic_serial_dev_setup(struct driver_param *param);

extern void generic_timer_dev_setup(struct driver_param *param);


/*
 * Register device drivers
 */
#define REGISTER_DEV_DRIVER(struct_name)                                \
    extern struct internal_dev_driver __##struct_name##_driver;         \
    register_dev_driver(&__##struct_name##_driver)


/*
 * Declare device drivers
 */
#define DECLARE_DEV_DRIVER(struct_name)                                 \
    struct internal_dev_driver __##struct_name##_driver

#define DECLARE_SERIAL_DRIVER(struct_name, desc, dt_compt,              \
                              dev_create, serial_setup, serial_putchar) \
    struct internal_dev_driver __##struct_name##_driver = {             \
        .name = (desc),                                                 \
        .probe_devtree_compatible = (dt_compt),                         \
        .create = (dev_create),                                         \
        .setup = generic_serial_dev_setup,                              \
        .serial = {                                                     \
            .setup = (serial_setup),                                    \
            .putchar = (serial_putchar),                                \
        },                                                              \
    }

#define DECLARE_TIMER_DRIVER(struct_name, desc, dt_compt,               \
                             dev_create, timer_setup, dev_start,        \
                             timer_power_on, clock_ty, clock_p,         \
                             int_s, int_h)                              \
    struct internal_dev_driver __##struct_name##_driver = {             \
        .name = (desc),                                                 \
        .probe_devtree_compatible = (dt_compt),                         \
        .create = (dev_create),                                         \
        .setup = generic_timer_dev_setup,                               \
        .start = dev_start,                                             \
        .timer = {                                                      \
            .setup = (timer_setup),                                     \
            .cpu_power_on = (timer_power_on),                           \
            .clock_type = (clock_ty),                                   \
            .clock_period = (clock_p),                                  \
            .int_seq = (int_s),                                         \
            .int_handler = (int_h),                                     \
        },                                                              \
    }

#endif
