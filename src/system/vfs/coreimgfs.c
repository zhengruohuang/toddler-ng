#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys.h>
#include <sys/api.h>
#include <fs/pseudo.h>

#include "libk/include/coreimg.h"


/*
 * FS structure
 */
static salloc_obj_t entry_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct pseudo_fs_node));
static struct pseudo_fs coreimg_fs;

static struct pseudo_fs_node *new_node(struct pseudo_fs_node *parent, const char *name, int type)
{
    struct pseudo_fs_node *node = salloc(&entry_salloc);
    pseudo_fs_node_setup(&coreimg_fs, node, name, type, 0, 0, 0);
    pseudo_fs_node_attach(&coreimg_fs, parent, node);

    return node;
}

static void setup_root()
{
    new_node(NULL, "/", VFS_NODE_DIR);
}

static void setup_file(const char *name, void *data, size_t size)
{
    struct pseudo_fs_node *node = new_node(coreimg_fs.root, name, VFS_NODE_FILE);
    pseudo_fs_set_data(node, PSEUDO_FS_DATA_FIXED, (void *)data, size);
}


/*
 * Ops
 */
static const struct fs_ops coreimgfs_ops = {
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
void init_coreimgfs()
{
    kprintf("Mouting coreimgfs\n");

    // Map and init core image
    void *ci = (void *)syscall_vm_map(VM_MAP_COREIMG, 0, 0);
    panic_if(!ci, "Unable to map core image!\n");

    init_libk_coreimg(ci);
    kprintf("Core image mapped @ %p, size: %u, num files: %u\n",
            ci, coreimg_size(), coreimg_file_count());

    // Construct FS structure
    setup_root();

    int num_files = coreimg_file_count();
    for (int i = 0; i < num_files; i++) {
        int ok = coreimg_is_checksum_correct(i);
        panic_if(!ok, "Coreimg corrupted!\n");

        char name[32];
        size_t size = 0;
        coreimg_get_filename(i, name, 32);
        void *data = coreimg_get_file2(i, &size);

        setup_file(name, data, size);
    }

    // Mount
    create_fs("/sys/coreimg", "coreimgfs", &coreimg_fs, &coreimgfs_ops, 1);
}
