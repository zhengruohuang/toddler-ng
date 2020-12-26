#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/refcount.h"
#include "libsys/include/syscall.h"
#include "libk/include/string.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/stdlib.h"


enum vfs_ops {
    VFS_OP_INVALID,

    VFS_OP_MOUNT,
    VFS_OP_UNMOUNT,

    VFS_OP_ACCESS,
    VFS_OP_RELEASE,
    VFS_OP_MOVE,

    VFS_OP_FILE_OPEN,
    VFS_OP_FILE_READ,
    VFS_OP_FILE_WRITE,
    VFS_OP_FILE_FLUSH,
    VFS_OP_FILE_CREATE,
    VFS_OP_FILE_TRUNCATE,
    VFS_OP_FILE_SYNC,

    VFS_OP_FILE_LINK,
    VFS_OP_FILE_UNLINK,

    VFS_OP_DIR_OPEN,
    VFS_OP_DIR_READ,
    VFS_OP_DIR_CREATE,
    VFS_OP_DIR_REMOVE,
    VFS_OP_DIR_SYNC,

    VFS_OP_CTRL_OPNEN,

    NUM_VFS_OPS,

    // getattr
    // mknod
    // chmod
    // chown
    // statfs
    // setxattr
    // getxattr
    // listxattr
    // remove xattr
    // init
    // distroy
    // fgetattr
    // lock
    // utimes
    // bmap
    // symlink
    // readlink
};

struct ventry;

struct mount_point {
    struct ventry *entry;
    char *name;

    ulong pid;
    ulong ops[NUM_VFS_OPS];
};

struct vnode {
    struct mount_point *mount;

    ref_count_t link_count;
    ref_count_t open_count;
};

struct ventry {
    struct ventry *parent;
    struct ventry *child;
    struct ventry *sibling;

    char *name;
    struct mount_point *mount;
    struct vnode *vnode;

    ref_count_t ref_count;
};


static salloc_obj_t ventry_salloc = SALLOC_CREATE(sizeof(struct ventry), 0, NULL, NULL);
static salloc_obj_t vnode_salloc = SALLOC_CREATE(sizeof(struct vnode), 0, NULL, NULL);
static salloc_obj_t mount_salloc = SALLOC_CREATE(sizeof(struct mount_point), 0, NULL, NULL);

static struct ventry *vroot = NULL;


static int compare_ventry_name(struct ventry *vent, const char *path, size_t seg_len)
{
    int c = strncmp(vent->name, path, seg_len);
    if (c) {
        return c;
    }

    return vent->name[seg_len] == '\0' ? 0 : 1;
}

struct ventry *lookup_ventry(struct ventry *cur_vent, const char *path, size_t seg_len)
{
    for (struct ventry *vent = cur_vent->child; vent; vent = vent->sibling) {
        if (!compare_ventry_name(vent, path, seg_len)) {
            return vent;
        }
    }

    return NULL;
}

static struct ventry *walk_path(struct ventry *root, const char *path,
                                int get_mount_point, int build_missing,
                                const char **fs_path)
{
    if (!path || *path == '\0') {
        return root;
    }

    struct ventry *last_mount_point = NULL;
    if (get_mount_point && root->mount) {
        last_mount_point = root;
        if (fs_path) *fs_path = path;
    }

    const char *cur_path = path;
    const char *next_path = strchr(cur_path, '/');

    struct ventry *prev_vent = root;
    struct ventry *cur_vent = NULL;

    do {
        size_t seg_len = 0;

        // something like ".../a/..."
        if (next_path) {
            seg_len = next_path - cur_path;
        }

        // something like ".../a"
        else {
            seg_len = strlen(cur_path);

            // something like ".../"
            if (!seg_len) {
                cur_vent = prev_vent;
                break;
            }
        }

        // Move to "a"
        cur_vent = lookup_ventry(prev_vent, cur_path, seg_len);
        if (!cur_vent) {
            if (build_missing) {
                // TODO
            } else {
                break;
            }
        }

        // Record last mount point
        if (get_mount_point && cur_vent->mount) {
            last_mount_point = cur_vent;
            if (fs_path) *fs_path = next_path;
        }

        // Next path
        cur_path = next_path;
        next_path = strchr(cur_path, '/');
        prev_vent = cur_vent;
    } while (cur_path);

    return get_mount_point ? last_mount_point : cur_vent;
}

static struct ventry *lookup_path(struct ventry *root, const char *path)
{
    return walk_path(root, path, 0, 0, NULL);
}

static struct ventry *lookup_mount_point(struct ventry *root, const char *path, const char **fs_path)
{
    return walk_path(root, path, 1, 0, fs_path);
}

static struct ventry *build_path(struct ventry *root, const char *path)
{
    return walk_path(root, path, 0, 1, NULL);
}


/*
 * Access
 */
static void inc_tree_open_count(struct ventry *vent)
{
}

static void dec_tree_open_count(struct ventry *vent)
{
}

struct ventry *vfs_access(const char *path)
{
    // TODO: relative path
    if (path[0] == '/') {
        path++;
    }

    // Path lookup
    struct ventry *vent = lookup_path(vroot, path);
    if (vent && vent->vnode) {
        inc_tree_open_count(vent);
        return vent;
    }

    // Access FS
    const char *fs_path = NULL;
    struct ventry *fs = lookup_mount_point(vroot, path, &fs_path);
    if (fs) {
        // TODO: send access message
        struct vnode *node = NULL;
        if (node) {
            vent = build_path(vroot, path);
            if (vent) {
                vent->vnode = node;
            }
        }
    }

    // Access failed
    if (!vent || !vent->vnode) {
        return NULL;
    }

    inc_tree_open_count(vent);
    return vent;
}

int vfs_release(struct ventry *vent)
{
    dec_tree_open_count(vent);
}


/*
 * Move
 */


/*
 * Mount
 */
int vfs_mount(struct ventry *vent)
{
}

int vfs_unmount(struct ventry *vent)
{
}


/*
 * File ops
 */


/*
 * Link
 */


/*
 * Dir ops
 */


/*
 * IO ops
 */


/*
 * Init
 */
void init_vfs()
{
}
