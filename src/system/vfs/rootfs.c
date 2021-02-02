#include <stdlib.h>
#include <stdio.h>
#include <sys/api.h>
#include <fs/pseudo.h>

#include "common/include/names.h"


/*
 * FS structure
 */
static salloc_obj_t entry_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct pseudo_fs_node));
static struct pseudo_fs root_fs;

static struct pseudo_fs_node *new_node(struct pseudo_fs_node *parent, const char *name, int type)
{
    struct pseudo_fs_node *node = salloc(&entry_salloc);
    pseudo_fs_node_setup(&root_fs, node, name, type, 0, 0, 0);
    pseudo_fs_node_attach(&root_fs, parent, node);

    return node;
}

static void set_node_data(struct pseudo_fs_node *node, const char *data, size_t size)
{
    pseudo_fs_set_data(node, PSEUDO_FS_DATA_FIXED, (void *)data, size);
}


/*
 * Ops
 */
static const struct fs_ops rootfs_ops = {
    .mount = pseudo_fs_mount,
    .unmount = pseudo_fs_unmount,

    .lookup = pseudo_fs_lookup,

    .acquire = pseudo_fs_acquire,
    .release = pseudo_fs_release,

    .file_open = pseudo_fs_file_open,
    .file_read = pseudo_fs_file_read,

    .dir_open = pseudo_fs_dir_open,
    .dir_read = pseudo_fs_dir_read,
    .dir_create = pseudo_fs_dir_create,
    .dir_remove = pseudo_fs_dir_remove,

    .symlink_read = pseudo_fs_symlink_read,
};


/*
 * Init
 */
void init_rootfs()
{
    kprintf("Mouting rootfs\n");

    // Init
    pseudo_fs_setup(&root_fs);

    // Root
    struct pseudo_fs_node *root = new_node(NULL, "/", VFS_NODE_DIR);

    // Subdirs
    new_node(root, "proc", VFS_NODE_DIR);
    new_node(root, "dev", VFS_NODE_DIR);
    new_node(root, "ipc", VFS_NODE_DIR);

    struct pseudo_fs_node *sys = new_node(root, "sys", VFS_NODE_DIR);
    new_node(sys, "coreimg", VFS_NODE_DIR);
    new_node(sys, "devtree", VFS_NODE_DIR);

    // File
    const char *about_data =
        "Toddler " ARCH_NAME "-" MACH_NAME "-" MODEL_NAME "\n"
        "Built on " __DATE__ " at " __TIME__ "\n"
        ;
    struct pseudo_fs_node *about = new_node(root, "about", VFS_NODE_FILE);
    set_node_data(about, about_data, strlen(about_data) + 1);

    const char *link_data = "/test/test2/test3";
    struct pseudo_fs_node *link = new_node(root, "link", VFS_NODE_SYMLINK);
    set_node_data(link, link_data, strlen(link_data) + 1);

    // Test
    const char *test_data =
        " 1 Welcome!\n 2 Welcome!\n 3 Welcome!\n 4 Welcome!\n 5 Welcome!\n"
        " 6 Welcome!\n 7 Welcome!\n 8 Welcome!\n 9 Welcome!\n10 Welcome!\n\n"
        "11 Welcome!\n12 Welcome!\n13 Welcome!\n14 Welcome!\n15 Welcome!\n"
        "16 Welcome!\n17 Welcome!\n18 Welcome!\n19 Welcome!\n20 Welcome!\n\n"
        "21 Welcome!\n22 Welcome!\n23 Welcome!\n24 Welcome!\n25 Welcome!\n"
        "26 Welcome!\n27 Welcome!\n28 Welcome!\n29 Welcome!\n30 Welcome!\n\n"
        "Yeah!\n\n";

    struct pseudo_fs_node *test = new_node(root, "test", VFS_NODE_DIR);
    struct pseudo_fs_node *test2 = new_node(test, "test2", VFS_NODE_DIR);
    struct pseudo_fs_node *test3 = new_node(test2, "test3", VFS_NODE_DIR);
    struct pseudo_fs_node *test4 = new_node(test3, "test4", VFS_NODE_FILE);
    set_node_data(test4, test_data, strlen(test_data) + 1);

    new_node(test, "sub1", VFS_NODE_DIR);
    new_node(test, "sub2", VFS_NODE_DIR);

    // Mount
    create_fs(NULL, "rootfs", &root_fs, &rootfs_ops, 1);
}
