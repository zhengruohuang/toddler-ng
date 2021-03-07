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

    union {
        struct {
            int num_int_cells;
        } intc;
    };
};

struct driver_int_encode {
    int size;
    void *data;
};

struct int_context;

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
            int (*int_handler)(struct driver_param *param, struct int_context *ictxt);
        } timer;

        struct {
            void (*setup_irq)(struct driver_param *param, int irq_seq);
            void (*enable_irq)(struct driver_param *param, struct int_context *ictxt, int irq_seq);
            void (*disable_irq)(struct driver_param *param, struct int_context *ictxt, int irq_seq);
            void (*end_irq)(struct driver_param *param, struct int_context *ictxt, int irq_seq);

            int (*irq_raw_to_seq)(struct driver_param *param, void *encode);

            void *(*create)(struct fw_dev_info *fw_info, struct driver_param *param);
            void (*setup)(struct driver_param *param);
            void (*start_cpu)(struct driver_param *param, int mp_seq, ulong mp_id, ulong entry);
            void (*cpu_power_on)(struct driver_param *param, int mp_seq, ulong mp_id);

            int int_seq;
            int (*find_pending_irq)(struct driver_param *param, struct int_context *ictxt);

            int max_num_devs;
            struct driver_param **devs;
        } intc;
    };
};

extern struct internal_dev_driver *get_all_internal_drivers();
extern void register_dev_driver(struct internal_dev_driver *drv);


/*
 * Generic driver interfaces
 */
extern void generic_serial_dev_setup(struct driver_param *param);
extern void generic_serial_dev_start(struct driver_param *param);

extern void generic_timer_dev_setup(struct driver_param *param);

extern void generic_intc_dev_setup(struct driver_param *param);
extern void generic_intc_dev_setup_int(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev);
extern void generic_intc_dev_eoi(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev);
extern void *generic_intc_dev_create(struct fw_dev_info *fw_info, struct driver_param *param);


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
        .start = generic_serial_dev_start,                              \
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
        .start = (dev_start),                                           \
        .timer = {                                                      \
            .setup = (timer_setup),                                     \
            .cpu_power_on = (timer_power_on),                           \
            .clock_type = (clock_ty),                                   \
            .clock_period = (clock_p),                                  \
            .int_seq = (int_s),                                         \
            .int_handler = (int_h),                                     \
        },                                                              \
    }

#define DECLARE_INTC_DRIVER(struct_name, desc, dt_compt,                \
                            intc_create, intc_setup, dev_start,         \
                            intc_start_cpu, intc_cpu_power_on,          \
                            intc_irq_raw_to_seq,                        \
                            intc_setup_irq, intc_enable_irq,            \
                            intc_disable_irq, intc_end_irq,             \
                            intc_pending_irq, max_irq_seq, int_s)       \
    static struct driver_param *__##struct_name##_devs[max_irq_seq];    \
    struct internal_dev_driver __##struct_name##_driver = {             \
        .name = (desc),                                                 \
        .probe_devtree_compatible = (dt_compt),                         \
        .create = generic_intc_dev_create,                              \
        .setup = generic_intc_dev_setup,                                \
        .setup_int = generic_intc_dev_setup_int,                        \
        .start = (dev_start),                                           \
        .eoi = generic_intc_dev_eoi,                                    \
        .intc = {                                                       \
            .setup_irq = (intc_setup_irq),                              \
            .enable_irq = (intc_enable_irq),                            \
            .disable_irq = (intc_disable_irq),                          \
            .end_irq = (intc_end_irq),                                  \
            .irq_raw_to_seq = (intc_irq_raw_to_seq),                    \
            .create = (intc_create),                                    \
            .setup = (intc_setup),                                      \
            .start_cpu = (intc_start_cpu),                              \
            .cpu_power_on = (intc_cpu_power_on),                        \
            .int_seq = (int_s),                                         \
            .find_pending_irq = (intc_pending_irq),                     \
            .max_num_devs = max_irq_seq,                                \
            .devs = __##struct_name##_devs,                             \
        },                                                              \
    }

#endif
