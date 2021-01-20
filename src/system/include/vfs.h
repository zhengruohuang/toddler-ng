#ifndef __SYSTEM_INCLUDE_VFS_H__
#define __SYSTEM_INCLUDE_VFS_H__


#include <stdint.h>
#include <atomic.h>
#include <kth.h>
#include <fs/op.h>


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

    rwlock_t rwlock;
};

struct vnode {
    ulong fs_id;
    int cacheable;

    struct mount_point *mount;

    ref_count_t link_count;
    ref_count_t open_count;
    rwlock_t rwlock;
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
    struct mount_point *from_mount;

    struct mount_point *mount;
    struct vnode *vnode;

    ref_count_t ref_count;
    rwlock_t rwlock;
};


/*
 * VFS
 */
extern void init_vfs();

extern struct ventry *vfs_acquire(const char *path);
extern int vfs_release(struct ventry *vent);

extern int vfs_mount(struct ventry *vent, const char *name, pid_t pid,
                     unsigned long opcode, unsigned long ops_ignore_map);

extern int vfs_dev_create(struct vnode *node, const char *name, unsigned int flags,
                          pid_t pid, unsigned long opcode);
extern int vfs_pipe_create(struct vnode *node, const char *name, unsigned int flags);

extern int vfs_file_open(struct vnode *node, ulong flags, ulong mode);
extern void vfs_file_read_forward(struct vnode *node, size_t count, size_t offset);
extern ssize_t vfs_file_write(struct vnode *node, const void *data, size_t count, size_t offset);

extern int vfs_dir_open(struct vnode *node, ulong mode);
extern void vfs_dir_read_forward(struct vnode *node, size_t count, ulong offset);


/*
 * Built-in pseudo file systems
 */
extern void init_rootfs();
extern void init_coreimgfs();


#endif
