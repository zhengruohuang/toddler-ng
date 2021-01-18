#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys.h>
#include <sys/api.h>
#include <fs/pseudo.h>

#include "libk/include/devtree.h"


/*
 * FS structure
 */
enum dtfs_node_type {
    DTFS_NODE_ROOT,
    DTFS_NODE_DEVICE,
    DTFS_NODE_PROP,
    DTFS_NODE_DRIVER,
    DTFS_NODE_PROP_PARENT,
    DTFS_NODE_DRIVER_PARENT,
};

struct dtfs_node {
    struct pseudo_fs_node fs_node;
    struct dtfs_node *dev;
};

struct devtree_fs {
    struct pseudo_fs fs_node;
};

static salloc_obj_t dtfs_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct dtfs_node));

static struct devtree_fs devtree_fs;


static struct dtfs_node *new_dtfs_node(struct dtfs_node *parent, const char *name, int type)
{
    int node_type = (type == DTFS_NODE_PROP || type == DTFS_NODE_DRIVER) ?
                    VFS_NODE_FILE : VFS_NODE_DIR;
    struct dtfs_node *node = salloc(&dtfs_salloc);
    pseudo_fs_node_setup(&devtree_fs.fs_node, &node->fs_node, name,
                         node_type, 0, 0, 0);
    pseudo_fs_node_attach(&devtree_fs.fs_node, &parent->fs_node, &node->fs_node);

    return node;
}

static struct dtfs_node *new_device_node(struct dtfs_node *parent, struct devtree_node *dt_node)
{
    const char *name = devtree_get_node_name(dt_node);
    struct dtfs_node *node = new_dtfs_node(parent, name, DTFS_NODE_DEVICE);
    return node;
}

static struct dtfs_node *new_prop_node(struct dtfs_node *parent, struct devtree_prop *dt_prop)
{
    const char *name = devtree_get_prop_name(dt_prop);
    void *data = devtree_get_prop_data(dt_prop);

    struct dtfs_node *node = new_dtfs_node(parent, name, DTFS_NODE_PROP);
    pseudo_fs_set_data(&node->fs_node, PSEUDO_FS_DATA_FIXED, data, dt_prop->len);

    return node;
}

static struct dtfs_node *new_device(struct dtfs_node *parent,
                                    struct devtree_node *dt_node)
{
    struct dtfs_node *fs_node = new_device_node(parent, dt_node);

    // Props
    struct dtfs_node *prop_fs_node = new_dtfs_node(fs_node, ".properties", DTFS_NODE_PROP_PARENT);
    for (struct devtree_prop *dt_prop = devtree_get_prop(dt_node); dt_prop;
         dt_prop = devtree_get_next_prop(dt_prop)
    ) {
        new_prop_node(prop_fs_node, dt_prop);
    }

    // Driver
    new_dtfs_node(fs_node, ".driver", DTFS_NODE_DRIVER_PARENT);

    return fs_node;
}

static struct dtfs_node *setup_root()
{
    return new_dtfs_node(NULL, "/", DTFS_NODE_ROOT);
}


/*
 * Ops
 */
static const struct fs_ops devtreefs_ops = {
    .mount = pseudo_fs_mount,
    .unmount = pseudo_fs_unmount,

    .lookup = pseudo_fs_lookup,

    .acquire = pseudo_fs_acquire,
    .release = pseudo_fs_release,

    .file_open = pseudo_fs_file_open,
    .file_read = pseudo_fs_file_read,

    .dir_open = pseudo_fs_dir_open,
    .dir_read = pseudo_fs_dir_read,
};


/*
 * Init
 */
static inline void _print_devtree_node_name(struct devtree_node *dt_node, int level)
{
    const char *name = devtree_get_node_name(dt_node);
    for (int i = 0; i < level; i++) kprintf("  ");
    kprintf("DT: %s\n", name);
}

static void visit_devtree_node(struct dtfs_node *parent_fs_node,
                               struct devtree_node *dt_node, int level)
{
    while (dt_node) {
        //_print_devtree_node_name(dt_node, level);

        struct dtfs_node *fs_node = new_device(parent_fs_node, dt_node);
        struct devtree_node *child_dt_node = devtree_get_child_node(dt_node);
        if (child_dt_node) {
            visit_devtree_node(fs_node, child_dt_node, level + 1);
        }

        dt_node = devtree_get_next_node(dt_node);
    }
}

void init_devtreefs()
{
    kprintf("Mouting devtreefs\n");

    // Map and init devtree
    void *dt = (void *)syscall_vm_map(VM_MAP_DEVTREE, 0, 0);
    panic_if(!dt, "Unable to map device tree!\n");

    open_libk_devtree(dt);
    struct devtree_head *head = dt;
    kprintf("Device tree mapped @ %p, root: %d, strs: %d, buf size: %lu\n",
            dt, head->root, head->strs, head->buf_size);

    // Construct FS structure
    struct dtfs_node *fs_node = setup_root();
    struct devtree_node *dt_node = devtree_get_root();
    visit_devtree_node(fs_node, dt_node, 0);

    // Mount
    create_fs("/sys/devtree", "devtreefs", &devtree_fs.fs_node, &devtreefs_ops, 1);
}
