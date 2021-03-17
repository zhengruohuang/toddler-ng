#include "common/include/inttypes.h"
#include "hal/include/devtree.h"
#include "hal/include/kprintf.h"
#include "hal/include/hal.h"
#include "hal/include/lib.h"
#include "hal/include/int.h"
#include "hal/include/dev.h"


enum dev_driver_type {
    DEV_DRIVER_NONE,
    DEV_DRIVER_HAL,
    DEV_DRIVER_EXTERNAL,
};

struct inttree_node;

struct dev_record {
    int fw_id;
    void *fw_node;

    int num_int_nodes;
    struct inttree_node *int_node;

    enum dev_driver_type type;
    struct internal_dev_driver *driver;
    struct driver_param driver_param;

    struct dev_record *next;

    struct dev_record *child;
    struct dev_record *sibling;
};

struct inttree_node {
    struct dev_record *dev;

    int fw_id;
    int fw_int_parent_id;
    void *fw_node;
    void *fw_int_parent_node;
    struct driver_int_encode fw_int_encode;

    int is_int_ctrl;
    int int_handler_seq;

    struct inttree_node *next_in_dev;
    struct inttree_node *next;

    struct inttree_node *parent;
    struct inttree_node *child;
    struct inttree_node *sibling;
};


static struct devtree_head *devtree = NULL;

static struct dev_record *dev_list = NULL;
static struct dev_record *dev_hierarchy = NULL;

static struct inttree_node *int_list = NULL;
static struct inttree_node *int_hierarchy = NULL;
static struct inttree_node **cpu_local_int_hierarchy = NULL;
static int num_global_int_roots = 0;


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
    kprintf("Register driver: %s\n", drv->name);

    drv->next = all_internal_drivers;
    all_internal_drivers = drv;
}

static void register_internal_drivers()
{
    REGISTER_DEV_DRIVER(i8259_intc);

    REGISTER_DEV_DRIVER(ns16550_uart);
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
            dev->driver->start(&dev->driver_param);
        }
    }
}

void start_all_devices_mp()
{
    ulong id = arch_get_cur_mp_id();
    int seq = arch_get_cur_mp_seq();
    drv_func_cpu_power_on(seq, id);
}


/*
 * Device enumeration
 */
static void probe_dev(struct dev_record *dev, struct fw_dev_info *fw_info)
{
    for (struct internal_dev_driver *drv = all_internal_drivers;
         drv; drv = drv->next
    ) {
        //kprintf("try drv @ %p, name @ %p, name: %s, probe @ %p\n", drv, drv->name, drv->name, drv->probe);
        int prob = FW_DEV_PROBE_FAILED;

        // Custom probe
        if (drv->probe) {
            prob = drv->probe(fw_info, &dev->driver_param);
        }

        // Devtree probe
        if (prob == FW_DEV_PROBE_FAILED &&
            drv->probe_devtree_compatible && fw_info->devtree_node &&
            match_devtree_compatibles(fw_info->devtree_node, drv->probe_devtree_compatible) &&
            devtree_get_enabled(fw_info->devtree_node)
        ) {
            prob = FW_DEV_PROBE_OK;
        }

        // Create and setup
        if (prob != FW_DEV_PROBE_FAILED) {
            dev->driver = drv;
            dev->driver_param.driver = drv;

            if (drv->create) {
                void *record = drv->create(fw_info, &dev->driver_param);
                dev->driver_param.record = record;
            }

            if (drv->setup) {
                drv->setup(&dev->driver_param);
            }

            break;
        }
    }
}

static struct dev_record *enum_devtree_node(struct devtree_node *node)
{
    // Set up the device node
    struct dev_record *dev = mempool_alloc(sizeof(struct dev_record));
    memzero(dev, sizeof(struct dev_record));

    int node_phandle = devtree_get_phandle(node);
    dev->fw_id = node_phandle;
    dev->fw_node = node;

    struct fw_dev_info fw_info = { .devtree_node = node };
    probe_dev(dev, &fw_info);

    dev->next = dev_list;
    dev_list = dev;

//     // TODO: deprecated
//     // Set up the associated interrupt node
//     dev->int_node = mempool_alloc(sizeof(struct inttree_node));
//     memzero(dev->int_node, sizeof(struct inttree_node));
//
//     dev->int_node->dev = dev;
//     dev->int_node->fw_id = devtree_get_phandle(node);
//     dev->int_node->fw_int_parent_id = devtree_get_int_parent(node);
//     dev->int_node->is_int_ctrl = devtree_is_intc(node);
//     dev->int_node->fw_int_encode.data = devtree_get_int_encode(node,
//                                    &dev->int_node->fw_int_encode.size);

    // Set up the associated interrupt node
    int next_pos = 0;
    do {
        int int_parent_phandle = 0;
        void *int_parent_node = NULL;
        void *encode = NULL;
        int encode_len = 0;
        next_pos = devtree_get_int(node, next_pos, &int_parent_phandle, &int_parent_node, &encode, &encode_len);

        if (next_pos >= 0) {
            struct inttree_node *int_node = mempool_alloc(sizeof(struct inttree_node));
            memzero(int_node, sizeof(struct inttree_node));

            int_node->dev = dev;
            int_node->fw_id = node_phandle;
            int_node->fw_int_parent_id = int_parent_phandle;
            int_node->fw_node = node;
            int_node->fw_int_parent_node = int_parent_node;
            int_node->fw_int_encode.data = encode;
            int_node->fw_int_encode.size = encode_len;
            int_node->is_int_ctrl = devtree_is_intc(node);

            // Fixup root intc which has no "interrupt-parent" node
            if (int_node->is_int_ctrl && !int_node->fw_int_parent_node) {
                int_node->fw_int_parent_id = int_node->fw_id;
                int_node->fw_int_parent_node = int_node->fw_node;
            }

//             kprintf("fw @ %p, id: %d, parent fw @ %p, id: %d\n",
//                     node, int_node->fw_id,
//                     int_node->fw_int_parent_node, int_node->fw_int_parent_id);

            int_node->next_in_dev = dev->int_node;
            dev->int_node = int_node;
            dev->num_int_nodes++;
        }
    } while (next_pos > 0);

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
static int is_int_parent(struct inttree_node *dev_int_node,
                         struct inttree_node *intc_int_node)
{
    return dev_int_node &&
            dev_int_node->fw_int_parent_node == intc_int_node->fw_node;
}

static void setup_inttree_node(struct dev_record *dev,
                               struct inttree_node *dev_int_node,
                               struct dev_record *intc_dev,
                               struct inttree_node *intc_int_node)
{
    if (dev == intc_dev) {
        if (int_hierarchy) {
            kprintf("Found more than one root intc!\n");
        }

        num_global_int_roots++;
        intc_int_node->sibling = int_hierarchy;
        int_hierarchy = intc_int_node;
        kprintf("Found root intc @ fw id: %d\n", intc_int_node->fw_id);
    } else {
        dev_int_node->sibling = intc_int_node->child;
        dev_int_node->parent = intc_int_node;
        intc_int_node->child = dev_int_node;

        if (intc_dev->driver && intc_dev->driver->setup_int &&
            dev->driver && dev->driver_param.int_seq
        ) {
            //kprintf("dev: %s\n", dev->driver->name);
            intc_dev->driver->setup_int(&intc_dev->driver_param,
                                        &dev_int_node->fw_int_encode,
                                        &dev->driver_param);
        }
    }
}

static void find_all_int_children(struct dev_record *intc_dev,
                                  struct inttree_node *intc_int_node)
{
    for (struct dev_record *dev = dev_list; dev; dev = dev->next) {
        for (struct inttree_node *dev_int_node = dev->int_node; dev_int_node;
             dev_int_node = dev_int_node->next_in_dev
        ) {
            if (is_int_parent(dev_int_node, intc_int_node)) {
                setup_inttree_node(dev, dev_int_node, intc_dev, intc_int_node);
            }
        }
    }
}

static void build_int_hierarchy()
{
    for (struct dev_record *dev = dev_list; dev; dev = dev->next) {
        for (struct inttree_node *int_node = dev->int_node; int_node;
             int_node = int_node->next_in_dev
        ) {
            if (int_node->is_int_ctrl) {
                find_all_int_children(dev, int_node);

                int_node->next = int_list;
                int_list = int_node;
            }
        }
    }

    panic_if(!int_hierarchy, "No root intc found!\n");
}

int set_cpu_local_intc(void *cpu_fw_node, int mp_seq)
{
    for (struct inttree_node *prev_intc = NULL, *intc = int_hierarchy; intc;
         prev_intc = intc, intc = intc->sibling
    ) {
        void *intc_fw_node = intc->dev->fw_node;
        if (devtree_in_subtree(cpu_fw_node, intc_fw_node)) {
            if (prev_intc) {
                prev_intc->sibling = intc->sibling;
            } else {
                int_hierarchy = intc->sibling;
            }

            intc->sibling = NULL;
            num_global_int_roots--;

            panic_if(!cpu_local_int_hierarchy, "CPU local intc not initialized!\n");
            panic_if(mp_seq >= get_num_cpus(), "Invalid MP seq: %d\n", mp_seq);
            cpu_local_int_hierarchy[mp_seq] = intc;

            kprintf("CPU local intc found for MP seq: %d, intc @ %p\n",
                    mp_seq, intc);
            return 0;
        }
    }

    return -1;
}

void setup_int_hierarchy_mp()
{
    int mp_seq = arch_get_cur_mp_seq();
    if (cpu_local_int_hierarchy && cpu_local_int_hierarchy[mp_seq]) {
        struct inttree_node *intc = cpu_local_int_hierarchy[mp_seq];
        struct internal_dev_driver *intc_drv = intc->dev->driver;

        if (intc_drv && intc_drv->intc.setup_cpu_local) {
            ulong mp_id = arch_get_cur_mp_id();
            intc_drv->intc.setup_cpu_local(&intc->dev->driver_param,
                                           mp_seq, mp_id);
        }
    }
}

void setup_int_hierarchy()
{
    int num_cpus = get_num_cpus();
    panic_if(num_global_int_roots > num_cpus,
             "Num root intc exceeding num cpus!\n");

    ulong size = sizeof(struct inttree_node *) * num_cpus;
    cpu_local_int_hierarchy = mempool_alloc(size);
    memzero(cpu_local_int_hierarchy, size);

    drv_func_detect_cpu_local_intc();
    panic_if(num_global_int_roots > 1,
             "At most one global root intc allowed!\n");

    setup_int_hierarchy_mp();
}

int handle_dev_int(struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    struct inttree_node *root_intc = int_hierarchy;

    int mp_seq = ictxt->mp_seq;
    if (cpu_local_int_hierarchy && cpu_local_int_hierarchy[mp_seq]) {
        root_intc = cpu_local_int_hierarchy[mp_seq];
    }

    panic_if(!root_intc, "Found no root intc!\n");

    int irq_seq = root_intc->dev->driver_param.int_seq;
    ictxt->param = &root_intc->dev->driver_param;
    return invoke_int_handler(irq_seq, ictxt, kdi);

//     int seq = int_hierarchy->dev->driver_param.int_seq;
//     ictxt->param = &int_hierarchy->dev->driver_param;
//     return invoke_int_handler(seq, ictxt, kdi);
}


/*
 * User-space
 */
static void *_user_int_register(struct dev_record *dev, int int_pos, ulong user_seq)
{
    if (!dev) {
        return NULL;
    }

    // Obtain int tree nodes
    panic_if(int_pos || dev->num_int_nodes > 1,
             "interrupts-extended not supported by user interrupt handlers!\n");
    struct inttree_node *int_node = dev->int_node;
    struct inttree_node *intc_int_node = int_node->parent;
    if (!intc_int_node || !intc_int_node->is_int_ctrl) {
        return NULL;
    }

    // Invoke intc driver and register user handler
    struct dev_record *intc_dev = intc_int_node->dev;
    struct internal_dev_driver *intc_drv = intc_dev->driver;
    if (intc_drv && intc_drv->setup_int) {
        intc_drv->setup_int(&intc_dev->driver_param,
                            &dev->int_node->fw_int_encode, &dev->driver_param);

        dev->driver_param.user_seq = user_seq;
        return dev;
    }

    return NULL;
}

void *user_int_register(ulong fw_id, int fw_pos, ulong user_seq)
{
    // Find the dev
    struct dev_record *dev = NULL;
    for (struct dev_record *d = dev_list; d; d = d->next) {
        if (d->fw_id == (int)fw_id) {
            dev = d;
            break;
        }
    }

    // Register
    return _user_int_register(dev, fw_pos, user_seq);
}

void *user_int_register2(const char *fw_path, int fw_pos, ulong user_seq)
{
    void *fw_node = devtree_walk(fw_path);

    // Find the dev
    struct dev_record *dev = NULL;
    for (struct dev_record *d = dev_list; d; d = d->next) {
        if (d->fw_node == fw_node) {
            dev = d;
            break;
        }
    }

    // Register
    return _user_int_register(dev, fw_pos, user_seq);
}

void user_int_eoi(void *hal_dev)
{
    if (!hal_dev) {
        return;
    }

    struct dev_record *dev = hal_dev;

    struct inttree_node *intc = dev->int_node->parent;
    panic_if(!intc || !intc->is_int_ctrl, "Dev parent must be int ctrl!\n");

    struct dev_record *intc_dev = intc->dev;
    struct internal_dev_driver *intc_drv = intc_dev->driver;
    panic_if(!intc_drv, "Int ctrl must have a driver!\n");

    if (intc_drv->eoi) {
        intc_drv->eoi(&intc_dev->driver_param,
            &dev->int_node->fw_int_encode, &dev->driver_param);
    }
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
