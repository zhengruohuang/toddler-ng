#ifndef __SYSTEM_INCLUDE_VFS_H__
#define __SYSTEM_INCLUDE_VFS_H__


#include "common/include/inttypes.h"
#include "common/include/refcount.h"


enum vfs_ops {
    VFS_OP_INVALID,

    VFS_OP_MOUNT,
    VFS_OP_UNMOUNT,

    VFS_OP_LOOKUP,
    VFS_OP_FORGET,

    VFS_OP_ACQUIRE,
    VFS_OP_RELEASE,
    VFS_OP_ACCESS,
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

    VFS_OP_SYMLINK_CREATE,
    VFS_OP_SYMLINK_READ,

    VFS_OP_DIR_OPEN,
    VFS_OP_DIR_READ,
    VFS_OP_DIR_CREATE,
    VFS_OP_DIR_REMOVE,
    VFS_OP_DIR_SYNC,

    VFS_OP_CTRL_OPEN,
    VFS_OP_CTRL_QUERY,
    VFS_OP_CTRL_READ,
    VFS_OP_CTRL_WRITE,

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
    // destroy
    // fgetattr
    // lock
    // utimes
    // bmap
    // symlink
    // readlink
};


struct ventry;

struct mount_point {
    struct ventry *entry;   // parent
    struct ventry *root;    // child

    char *name;
    ulong root_fs_id;
    int read_only;
    u32 ops_ignore_map;

    struct {
        ulong pid;
        ulong opcode;
    } ipc;
};

struct vnode {
    ulong fs_id;
    int cacheable;

    struct mount_point *mount;

    ref_count_t link_count;
    ref_count_t open_count;
};

struct ventry {
    struct ventry *parent;
    struct ventry *child;
    struct {
        struct ventry *prev;
        struct ventry *next;
    } sibling;

    int is_root;
    char *name;
    ulong fs_id;
    int cacheable;

    struct mount_point *mount;
    struct vnode *vnode;

    ref_count_t ref_count;
};


/*
 * VFS
 */
extern void init_vfs();
extern void init_vfs_api();

extern struct ventry *vfs_acquire(const char *path);
extern int vfs_release(struct ventry *vent);

extern int vfs_mount(struct ventry *vent, const char *name,
                     ulong pid, ulong opcode, ulong root_fs_id,
                     int read_only, u32 ops_ignore_map);

extern int vfs_file_open(struct vnode *node, ulong flags, ulong mode);
extern ssize_t vfs_file_read(struct vnode *node, size_t offset, char *buf, size_t buf_size);
extern ssize_t vfs_file_write(struct vnode *node, size_t offset, char *buf, size_t buf_size);
extern void vfs_file_read_forward(struct vnode *node, size_t count, size_t offset);

extern int vfs_dir_open(struct vnode *node, ulong mode);
void vfs_dir_read_forward(struct vnode *node, size_t count, ulong offset);


/*
 * RootFS
 */
extern void init_rootfs();


/*
 * CoreFS
 */


/*
 * ProcFS
 */


/*
 * DevFS
 */


#endif
