#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>

#include "common/include/inttypes.h"
#include "system/include/vfs.h"


static salloc_obj_t ventry_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct ventry));
static salloc_obj_t vnode_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct vnode));
static salloc_obj_t mount_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct mount_point));

static struct ventry *vroot = NULL;


/*
 * Helper
 */
static inline int ignore_op_ipc(struct mount_point *mount, int op)
{
    return mount->ops_ignore_map & (0x1 << op);
}


/*
 * Mount
 */
static struct mount_point *find_mount_point(struct ventry *vent)
{
    for (; vent; vent = vent->parent) {
        if (vent->mount) {
            return vent->mount;
        }
    }

    return NULL;
}


/*
 * Ventry
 */
static void attach_ventry(struct ventry *parent, struct ventry *vent)
{
    ref_count_inc(&parent->ref_count);

    vent->child = NULL;
    vent->parent = parent;
    vent->sibling.prev = NULL;
    vent->sibling.next = parent->child;
    if (parent->child) parent->child->sibling.prev = vent;
    parent->child = vent;
}

static struct ventry *detach_ventry(struct ventry *vent)
{
    struct ventry *parent = vent->parent;
    if (!parent) {
        return NULL;
    }

    if (vent->sibling.prev) {
        vent->sibling.prev->sibling.next = vent->sibling.next;
    }
    if (vent->sibling.next) {
        vent->sibling.next->sibling.prev = vent->sibling.prev;
    }
    if (parent->child == vent) {
        parent->child = vent->sibling.next;
    }

    ref_count_dec(&parent->ref_count);
    return parent;
}


/*
 * Trim
 */
static void forget_ipc(struct ventry *vent)
{
    struct mount_point *mount = find_mount_point(vent);
    if (ignore_op_ipc(mount, VFS_OP_FORGET)) {
        return;
    }

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FORGET);
    msg_append_param(msg, vent->fs_id);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);
}

static struct ventry *trim_ventries(struct ventry *vent)
{
    while (vent && vent->parent && !vent->is_root &&
           ref_count_is_zero(&vent->ref_count)
    ) {
        if (vent->vnode) {
            // Make sure the file is closed
            if (!ref_count_is_zero(&vent->vnode->open_count)) {
                // TODO: panic
                kprintf("PANIC: Vnode open count mismatch!\n");
                return NULL;
            }

            sfree(vent->vnode);
            vent->vnode = NULL;
        }

        struct ventry *parent_vent = detach_ventry(vent);

        //kprintf("detach: %s, parent->child @ %p\n",
        //        vent->name, parent_vent->child);

        forget_ipc(vent);
        sfree(vent);
        vent = parent_vent;
    }

    return vent;
}


/*
 * Lookup
 */
static struct ventry *lookup_ipc(struct mount_point *mount,
                                 struct ventry *cur_vent,
                                 const char *path, size_t seg_len)
{
    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_LOOKUP);
    msg_append_param(msg, cur_vent->mount == mount ?
                          mount->root_fs_id : cur_vent->fs_id);
    msg_append_str(msg, (void *)path, seg_len);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    // Response
    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return NULL;
    }

    struct ventry *vent = salloc(&ventry_salloc);
    vent->fs_id = msg_get_param(msg, 1);
    vent->cacheable = msg_get_int(msg, 2);

    vent->mount = NULL;
    vent->vnode = NULL;
    ref_count_init(&vent->ref_count, 0);

    vent->name = malloc(seg_len + 1);
    memcpy(vent->name, path, seg_len);
    vent->name[seg_len] = '\0';
    vent->from_mount = mount;

    //kprintf("vent @ %p, lookup: %s, fs_id: %lu, cache: %d\n",
    //        vent, vent->name, vent->fs_id, vent->cacheable);

    attach_ventry(cur_vent, vent);
    return vent;
}

static int compare_ventry_name(struct ventry *vent, const char *path, size_t seg_len)
{
    int c = strncmp(vent->name, path, seg_len);
    if (c) {
        return c;
    }

    return vent->name[seg_len] == '\0' ? 0 : 1;
}

static struct ventry *lookup_ventry(struct mount_point *mount,
                                    struct ventry *cur_vent,
                                    const char *path, size_t seg_len)
{
    // "." and ".."
    if (seg_len == 1 && path && path[0] == '.') {
        return cur_vent;
    } else if (seg_len == 2 && path && path[0] == '.' && path[1] == '.') {
        if (!cur_vent->parent) {
            return cur_vent;
        }

        if (cur_vent->is_root) {
            cur_vent = cur_vent->parent;
        }
        return cur_vent->parent ? cur_vent->parent : cur_vent;
    }

    //kprintf("lookup path: %s, len: %lu, fs id: %lu\n",
    //       path, seg_len, cur_vent->fs_id);

    // Cached ventry
    for (struct ventry *vent = cur_vent->child; vent; vent = vent->sibling.next) {
        if (!compare_ventry_name(vent, path, seg_len)) {
            return vent;
        }
    }

    // Look up in FS
    return lookup_ipc(mount, cur_vent, path, seg_len);
}

static struct ventry *walk_path(struct ventry *root, const char *path)
{
    //kprintf("Walk path!\n");

    if (!root->mount) {
        kprintf("Root must be a mount point!\n");
        return NULL;
    }

    struct mount_point *cur_mount = root->mount;

    struct ventry *prev_vent = cur_mount->root; // root;
    struct ventry *cur_vent = NULL;

    if (!path || *path == '\0') {
        rwlock_rlock(&prev_vent->rwlock);
        return prev_vent;
    }

    const char *cur_path = path;
    const char *next_path = strchr(cur_path, '/');

    rwlock_wlock(&cur_mount->rwlock);

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

        //kprintf("Cur: %s, next: %s, strchr @ %p\n",
        //      cur_path, next_path ? next_path : "(NULL)",
        //      strchr(cur_path, '/'));

        // Move to "a"
        cur_vent = lookup_ventry(cur_mount, prev_vent, cur_path, seg_len);
        if (!cur_vent) {
            trim_ventries(prev_vent);
            break;
        }

        // Encountered a new mount point
        if (cur_vent->mount) {
            struct mount_point *last_mount = cur_mount;
            cur_mount = cur_vent->mount;
            cur_vent = cur_mount->root;

            rwlock_wlock(&cur_mount->rwlock);
            rwlock_wunlock(&last_mount->rwlock);
        }

        // Next path
        prev_vent = cur_vent;
        cur_path = next_path;
        while (cur_path && *cur_path == '/') {
            // skip '/'
            cur_path++;
        }

        next_path = strchr(cur_path, '/');
    } while (cur_path);

    //kprintf("Done walk path @ %p: %s\n", cur_vent, cur_vent->name);

    if (cur_vent) {
        rwlock_rlock(&cur_vent->rwlock);
    }
    rwlock_wunlock(&cur_mount->rwlock);

    return cur_vent;
}


/*
 * Access
 */
// static inline void inc_tree_open_count(struct ventry *vent)
// {
//     for (; vent; vent = vent->parent) {
//         ref_count_inc(&vent->ref_count);
//     }
// }
//
// static inline void dec_tree_open_count(struct ventry *vent)
// {
//     for (; vent; vent = vent->parent) {
//         ref_count_dec(&vent->ref_count);
//     }
// }

static struct vnode *acquire_ipc(struct ventry *vent)
{
    struct mount_point *mount = find_mount_point(vent);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_ACQUIRE);
    msg_append_param(msg, vent->fs_id);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    // Response
    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return NULL;
    }

    struct vnode *node = salloc(&vnode_salloc);
    node->fs_id = msg_get_param(msg, 1);
    node->cacheable = msg_get_int(msg, 2);

    node->mount = mount;
    vent->vnode = node;

    ref_count_init(&node->open_count, 0);
    ref_count_init(&node->link_count, 0);

    return node;
}

static void release_ipc(struct ventry *vent)
{
    struct mount_point *mount = vent->vnode->mount;
    if (ignore_op_ipc(mount, VFS_OP_RELEASE)) {
        return;
    }

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_RELEASE);
    msg_append_param(msg, vent->fs_id);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);
}

struct ventry *vfs_acquire(const char *path)
{
    // TODO: relative path
    while (path && *path == '/') {
        // skip '/'
        path++;
    }

    // Path walk
    struct ventry *vent = walk_path(vroot, path);
    if (!vent) {
        return NULL;
    }

    // Acquire node
    if (!vent->vnode) {
        struct vnode *node = vent->vnode = acquire_ipc(vent);
        if (!node) {
            trim_ventries(vent);
            return NULL;
        }
    }

    ref_count_inc(&vent->ref_count);
    ref_count_inc(&vent->vnode->open_count);

    rwlock_runlock(&vent->rwlock);

    return vent;
}

int vfs_release(struct ventry *vent)
{
    if (!vent) {
        return -1;
    }

    rwlock_rlock(&vent->rwlock);
    ref_count_dec(&vent->vnode->open_count);
    ref_count_dec(&vent->ref_count);

    if (ref_count_is_zero(&vent->vnode->open_count)) {
        release_ipc(vent);
        //dec_tree_open_count(vent);

        rwlock_wlock(&vent->from_mount->rwlock);
        trim_ventries(vent);
        rwlock_wunlock(&vent->from_mount->rwlock);
    }

    return 0;
}


/*
 * Move
 */


/*
 * Mount
 */
int vfs_mount(struct ventry *vent, const char *name, pid_t pid,
              unsigned long opcode, unsigned long ops_ignore_map)
{
    if (!vent) {
        vent = vroot;
    }

    if (vent->mount) {
        // TODO: panic
        kprintf("PANIC: double mount!\n");
        return -1;
    }

    if (vent->child) {
        kprintf("Unable to mount on a non-empty dir\n");
        return -1;
    }

    // TODO: lock

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_MOUNT);
    syscall_ipc_popup_request(pid, opcode);

    // Response
    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }
    unsigned long root_fs_id = msg_get_param(msg, 1);
    int read_only = msg_get_int(msg, 2);

    // Mount
    struct mount_point *m = vent->mount = salloc(&mount_salloc);
    m->entry = vent;
    m->name = strdup(name);
    m->ipc.pid = pid;
    m->ipc.opcode = opcode;
    m->root_fs_id = root_fs_id;
    m->read_only = read_only;
    m->ops_ignore_map = ops_ignore_map;

    struct ventry *mroot = m->root = salloc(&ventry_salloc);
    mroot->fs_id = root_fs_id;
    mroot->cacheable = 1;
    mroot->name = strdup("/");
    mroot->parent = vent;
    mroot->is_root = 1;
    mroot->from_mount = m;
    ref_count_init(&mroot->ref_count, 1);

    kprintf("Mounted %s\n", name);

    // TODO: unlock

    return 0;
}

int vfs_unmount(struct ventry *vent)
{
    return -1;
}


/*
 * Special node
 */
int vfs_dev_create(struct vnode *node, const char *name, unsigned int flags,
                   pid_t pid, unsigned long opcode)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_DEV_CREATE)) {
        return -1;
    }

    char *name_dup = strdup(name);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_DEV_CREATE);
    msg_append_param(msg, node->fs_id);
    msg_append_str(msg, name_dup, 0);
    msg_append_param(msg, flags);
    msg_append_param(msg, pid);
    msg_append_param(msg, opcode);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    free(name_dup);

    return 0;
}

int vfs_pipe_create(struct vnode *node, const char *name, unsigned int flags)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_PIPE_CREATE)) {
        return -1;
    }

    char *name_dup = strdup(name);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_PIPE_CREATE);
    msg_append_int(msg, VFS_NODE_PIPE);
    msg_append_param(msg, node->fs_id);
    msg_append_str(msg, name_dup, 0);
    msg_append_param(msg, flags);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    free(name_dup);

    return 0;
}


/*
 * File ops
 */
int vfs_file_open(struct vnode *node, ulong flags, ulong mode)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_FILE_OPEN)) {
        return 0;
    }

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FILE_OPEN);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, flags);
    msg_append_param(msg, mode);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    return 0;
}

void vfs_file_read_forward(struct vnode *node, size_t count, size_t offset)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_FILE_READ)) {
        return;
    }

    rwlock_rlock(&node->rwlock);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FILE_READ);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    rwlock_runlock(&node->rwlock);
}

ssize_t vfs_file_write(struct vnode *node, const void *data, size_t count, size_t offset)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_FILE_WRITE)) {
        return 0;
    }

    rwlock_wlock(&node->rwlock);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FILE_WRITE);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    msg_append_data(msg, data, count);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    rwlock_wunlock(&node->rwlock);

    // Response
    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    size_t write_count = msg_get_param(msg, 1);
    return write_count;
}


/*
 * Link
 */


/*
 * Dir ops
 */
int vfs_dir_open(struct vnode *node, ulong flags)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_DIR_OPEN)) {
        return 0;
    }

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_DIR_OPEN);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, flags);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    return 0;
}

void vfs_dir_read_forward(struct vnode *node, size_t count, ulong offset)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_DIR_READ)) {
        return;
    }

    rwlock_rlock(&node->rwlock);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_DIR_READ);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    rwlock_runlock(&node->rwlock);
}


/*
 * IO ops
 */


/*
 * Real path
 */
int vfs_real_path(struct ventry *vent, char *buf, size_t buf_size)
{
    kprintf("here??\n");

    // "/"
    if (vent->is_root && !vent->from_mount) {
        if (!buf || buf_size < 2) {
            return -1;
        }

        buf[0] = '/';
        buf[1] = '\0';
        return 0;
    }

    // total length
    size_t real_size = 1; // '\0'
    for (struct ventry *e = vent; e; e = e->parent) {
        if (e->is_root && e->from_mount) {
            e = e->from_mount->entry;
        }

        if (e->parent) {
            real_size += strlen(e->name) + 1;
        }
    }

    if (real_size > buf_size) {
        return -1;
    }

    // copy
    buf[real_size - 1] = '\0';
    size_t pos = real_size - 1;
    for (struct ventry *e = vent; e; e = e->parent) {
        if (e->is_root && e->from_mount) {
            e = e->from_mount->entry;
        }

        if (e->parent) {
            size_t part_len = strlen(e->name);
            pos -= part_len;
            memcpy(buf + pos, e->name, part_len);
            pos--;
            buf[pos] = '/';
        }
    }

    kprintf("pos: %lu, real path: %s\n", pos, buf);
    return 0;
}


/*
 * Init
 */
void init_vfs()
{
    vroot = salloc(&ventry_salloc);
    vroot->name = "/";
    vroot->cacheable = 1;
    vroot->is_root = 1;
    ref_count_init(&vroot->ref_count, 1);
}
