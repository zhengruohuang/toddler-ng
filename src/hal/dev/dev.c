#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"
#include "hal/include/devtree.h"
#include "hal/include/kprintf.h"


enum dev_driver_type {
    DEV_DRIVER_HAL,
    DEV_DRIVER_EXTERNAL,
};

struct inttree_node;

struct dev_record {
    struct inttree_node *int_node;

    enum dev_driver_type type;
    struct internal_dev_driver *driver;
    void *driver_param;

    struct dev_record *next;

    struct dev_record *child;
    struct dev_record *sibling;
};

struct inttree_node {
    struct dev_record *dev;

    int fw_id;
    int fw_int_parent_id;
    int fw_int_encode_count;
    int *fw_int_encode;

    int is_int_ctrl;
    int int_handler_seq;

    struct inttree_node *next;

    struct inttree_node *child;
    struct inttree_node *sibling;
};


static struct devtree_head *devtree = NULL;

static struct dev_record *dev_list = NULL;
static struct dev_record *dev_hierarchy = NULL;

static struct inttree_node *int_list = NULL;
static struct inttree_node *int_hierarchy = NULL;


/*
 * Driver registration
 */
static struct internal_dev_driver *all_internal_drivers = NULL;

struct internal_dev_driver *get_all_internal_drivers()
{
    return all_internal_drivers;
}

void register_dev_driver(struct internal_dev_driver *drv)
{
    if (all_internal_drivers) {
        drv->next = all_internal_drivers;
    }
    all_internal_drivers = drv;
}

static void register_internal_drivers()
{
}


/*
 * Match
 */
int match_devtree_compatible(struct devtree_node *node, const char *drv)
{
    int idx = 0;
    do {
        char *dev_name = NULL;
        idx = devtree_get_compatible(node, idx, &dev_name);
        //kprintf("%s\n", idx >= 0 ? dev_name : "none");
        if (idx >= 0 && !strcmp(dev_name, drv)) {
            return 1;
        }
    } while (idx > 0);

    return 0;
}

int match_devtree_compatibles(struct devtree_node *node, const char *drvs[])
{
    if (!drvs) {
        return 0;
    }

    for (int i = 0; drvs[i]; i++) {
        if (match_devtree_compatible(node, drvs[i])) {
            return 1;
        }
    }

    return 0;
}


/*
 * Start all devices
 */
void start_all_devices()
{
    for (struct dev_record *dev = dev_list; dev; dev = dev->next) {
        if (dev->driver && dev->driver->start) {
            dev->driver->start(dev->driver_param);
        }
    }
}


/*
 * Device enumeration
 */
static void probe_dev(struct dev_record *dev, struct fw_dev_info *fw_info)
{
    for (struct internal_dev_driver *drv = all_internal_drivers;
         drv; drv = drv->next
    ) {
        if (drv->probe) {
            int prob = drv->probe(fw_info, &dev->driver_param);
            if (prob != FW_DEV_PROBE_FAILED) {
                dev->driver = drv;
                if (drv->setup) {
                    drv->setup(dev->driver_param);
                }
                break;
            }
        }
    }
}

static struct dev_record *enum_devtree_node(struct devtree_node *node)
{
    // Set up the device node
    struct dev_record *dev = mempool_alloc(sizeof(struct dev_record));
    memzero(dev, sizeof(sizeof(struct dev_record)));

    struct fw_dev_info fw_info = { .devtree_node = node };
    probe_dev(dev, &fw_info);

    dev->next = dev_list;
    dev_list = dev;

    // Set up the associated interrupt node
    dev->int_node = mempool_alloc(sizeof(struct inttree_node));
    memzero(dev->int_node, sizeof(struct inttree_node));

    dev->int_node->dev = dev;
    dev->int_node->fw_id = devtree_get_phandle(node);
    dev->int_node->fw_int_parent_id = devtree_get_int_parent(node, 0);
    dev->int_node->is_int_ctrl = devtree_is_intc(node);
    dev->int_node->fw_int_encode = devtree_get_int_encode(node,
                                   &dev->int_node->fw_int_encode_count);

    // Keep traversing
    struct devtree_node *child = devtree_get_child_node(node);
    while (child) {
        struct dev_record *child_dev = enum_devtree_node(child);
        if (child_dev) {
            child_dev->sibling = dev->child;
            dev->child = child_dev;
        }
        child = devtree_get_next_node(child);
    }

    return dev;
}

static void enum_devtree()
{
    struct devtree_node *node = devtree_get_root();
    panic_if(!node, "Unable to open devtree!");

    dev_hierarchy = enum_devtree_node(node);
}

static void enum_all_devices()
{
    enum_devtree();
}


/*
 * Int tree
 */
static int is_int_parent(struct dev_record *dev, struct dev_record *intc_dev)
{
    return intc_dev->int_node->fw_id != -1 &&
           dev->int_node->fw_int_parent_id == intc_dev->int_node->fw_id;
}

static void find_all_int_children(struct dev_record *intc_dev)
{
    for (struct dev_record *dev = dev_list; dev; dev = dev->next) {
        if (is_int_parent(dev, intc_dev)) {
            if (dev == intc_dev) {
                panic_if(int_hierarchy, "Only one root intc allowed!");
                int_hierarchy = intc_dev->int_node;
            } else {
                dev->int_node->sibling = intc_dev->int_node->child;
                intc_dev->int_node->child = dev->int_node;
            }
        }
    }
}

static void build_int_hierarchy()
{
    for (struct dev_record *dev = dev_list; dev; dev = dev->next) {
        if (dev->int_node->is_int_ctrl) {
            find_all_int_children(dev);

            dev->int_node->next = int_list;
            int_list = dev->int_node;

            if (dev->driver && dev->driver->setup_int) {
                dev->driver->setup_int(dev->driver_param);
            }
        }
    }

    panic_if(!int_hierarchy, "No root intc found!\n");
}


/*
 * Init
 */
void init_dev()
{
    struct loader_args *largs = get_loader_args();
    devtree = largs->devtree;
    open_libk_devtree(devtree);

    register_internal_drivers();
    arch_register_drivers();

    enum_all_devices();
    build_int_hierarchy();
}
