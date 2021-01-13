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

    int (*probe)(struct fw_dev_info *fw_info, struct driver_param *param);
    void (*setup)(struct driver_param *param);
    void (*setup_int)(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev);
    void (*start)(struct driver_param *param);
    void (*eoi)(struct driver_param *param, struct driver_int_encode *encode, struct driver_param *dev);
};


#define DECLARE_DEV_DRIVER(name)                            \
    struct internal_dev_driver __##name##_driver

#define REGISTER_DEV_DRIVER(name)                           \
    extern struct internal_dev_driver __##name##_driver;    \
    register_dev_driver(&__##name##_driver)


extern struct internal_dev_driver *get_all_internal_drivers();
extern void register_dev_driver(struct internal_dev_driver *drv);


#endif
