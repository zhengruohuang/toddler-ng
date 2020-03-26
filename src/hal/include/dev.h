#ifndef __HAL_INCLUDE_DEV__
#define __HAL_INCLUDE_DEV__


#include "common/include/devtree.h"


/*
 * Device driver
 */
struct fw_dev_info {
    void *devtree_node;
};

enum fw_dev_probe_result {
    FW_DEV_PROBE_FAILED,
    FW_DEV_PROBE_OK,
};

struct internal_dev_driver {
    const char *name;
    struct internal_dev_driver *next;

    int (*probe)(struct fw_dev_info *fw_info, void **param);
    void (*setup)(void *param);
    void (*setup_int)(void *param);
    void (*start)(void *param);
};

extern struct internal_dev_driver *get_all_internal_drivers();
extern void register_dev_driver(struct internal_dev_driver *drv);

#define DECLARE_DEV_DRIVER(name)                            \
    struct internal_dev_driver __##name##_driver

#define REGISTER_DEV_DRIVER(name)                           \
    extern struct internal_dev_driver __##name##_driver;    \
    register_dev_driver(&__##name##_driver)


extern int match_devtree_compatible(struct devtree_node *node, const char *drv);
extern int match_devtree_compatibles(struct devtree_node *node, const char *drvs[]);

extern void start_all_devices();

extern void init_dev();


/*
 * Device access window
 */
enum dev_pfn_cache_query_type {
    DEV_PFN_UNCACHED    = 0,
    DEV_PFN_CACHED      = 1,
    DEV_PFN_ANY_CACHED  = 2,
};

extern ulong get_dev_access_window(ulong paddr, ulong size, int cached);


#endif
