#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic.h>
#include <assert.h>
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
                kprintf("PANIC: Vnode open count mismatch: %lu\n",
                        vent->vnode->open_count.value);
                return NULL;
            }

            sfree(vent->vnode);
            vent->vnode = NULL;
        }

        struct ventry *parent_vent = detach_ventry(vent);

        //kprintf("detach: %s, parent->child @ %p\n",
        //        vent->name, parent_vent->child);

        forget_ipc(vent);

        if (vent->name) {
            free(vent->name);
        }
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

    int node_type = msg_get_int(msg, 3);
    vent->is_symlink = node_type == VFS_NODE_SYMLINK;

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

static struct ventry *_try_walk_path(struct ventry *root, const char *path,
                                     const char **remain_path)
{
    if (!root->mount) {
        kprintf("Root must be a mount point!\n");
        return NULL;
    }

    struct mount_point *cur_mount = root->mount;

    struct ventry *prev_vent = cur_mount->root;
    struct ventry *cur_vent = NULL;

    if (!path || *path == '\0') {
        rwlock_wlock(&prev_vent->rwlock);
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
    } while (cur_path && !cur_vent->is_symlink);

    //kprintf("Done walk path @ %p: %s\n", cur_vent, cur_vent->name);

    if (cur_vent) {
        if (cur_vent->is_symlink && remain_path) {
            *remain_path = cur_path;
        }

        rwlock_wlock(&cur_vent->rwlock);
    }
    rwlock_wunlock(&cur_mount->rwlock);

    return cur_vent;
}

static struct ventry *walk_path(struct ventry *root, const char *path)
{
    struct ventry *vent = _try_walk_path(root, path, &path);

    int level = 0;
    char *prev_path_buf = NULL;
    while (vent && vent->is_symlink) {
        kprintf("Symlink @ %s, remain path: %s\n", vent->name, path ? path : "");

        // Read symlink
        // FIXME: Symlink currently must be abs path
        // FIXME: path_buf size may not be enough to hold the entire symlink
        char *path_buf = malloc(512);
        vfs_symlink_read_to_buf(vent, path_buf, 512);
        kprintf("link to: %s\n", path_buf);

        // Append remain path to the abs path
        // TODO: check if buf size is enough
        size_t len = strlen(path_buf);
        path_buf[len] = '/';
        if (path)
            strcpy(&path_buf[len + 1], path);

        // Done with current vent
        rwlock_wunlock(&vent->rwlock);
        trim_ventries(vent);

        if (prev_path_buf) {
            free(prev_path_buf);
        }
        prev_path_buf = path_buf;

        // Walk again
        if (++level >= 10) {
            vent = NULL;
        } else {
            const char *link_to_path = path_buf;
            while (link_to_path && *link_to_path == '/') {
                // skip '/'
                link_to_path++;
            }
            vent = _try_walk_path(root, link_to_path, &path);
        }
    }

    if (prev_path_buf) {
        free(prev_path_buf);
    }

    return vent;
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
    node->type = msg_get_int(msg, 3);

    node->mount = mount;
    vent->vnode = node;

    ref_count_init(&node->open_count, 0);
    ref_count_init(&node->link_count, 0);

    sema_init_default(&node->pipe.reader);
    sema_init_default(&node->pipe.writer);
    mutex_init(&node->pipe.lock);

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
            rwlock_wunlock(&vent->rwlock);
            trim_ventries(vent);
            return NULL;
        }
    }

    ref_count_inc(&vent->ref_count);
    ref_count_inc(&vent->vnode->open_count);

    rwlock_wunlock(&vent->rwlock);

    return vent;
}

int vfs_release(struct ventry *vent)
{
    if (!vent) {
        return -1;
    }

    rwlock_rlock(&vent->rwlock);
    ulong remain_count = ref_count_sub(&vent->vnode->open_count, 1);
    ref_count_dec(&vent->ref_count);

    release_ipc(vent);

    if (remain_count == 1) {
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
 * Pipe ops
 */
static void vfs_pipe_read(struct vnode *node, size_t count)
{
    //kprintf("Pipe read @ %p\n", node);
    sema_wait(&node->pipe.writer);
    //kprintf("After read writer wait\n");

    // Response
    msg_t *msg = get_empty_msg();           //
    msg_append_int(msg, 0);                 // ok
    msg_append_param(msg, 0);               // real size
    msg_append_int(msg, 0);                 // more

    // Buf size
    size_t req_count = node->pipe.count - node->pipe.offset;
    if (req_count > count) {
        req_count = count;
    }

    size_t max_size = msg_remain_data_size(msg) - 32ul; // extra safe
    if (req_count > max_size) {
        req_count = max_size;
    }

    // Data
    msg_append_data(msg, node->pipe.data + node->pipe.offset, req_count);
    node->pipe.offset += req_count;
    atomic_mb();

    // Update msg
    msg_set_param(msg, 1, req_count);

    sema_post(&node->pipe.reader);
    //kprintf("After read reader post\n");
}

static ssize_t vfs_pipe_write(struct vnode *node, const void *data, size_t count)
{
    ssize_t write_count = -1;

    // Only one writer is allowed at a time
    mutex_exclusive(&node->pipe.lock) {
        //kprintf("Pipe write @ %p\n", node);
        node->pipe.data = data;
        node->pipe.count = count;
        node->pipe.offset = 0;
        sema_post(&node->pipe.writer);
        //kprintf("After write writer post\n");

        sema_wait(&node->pipe.reader);
        //kprintf("After write reader wait\n");

        atomic_mb();
        write_count = node->pipe.offset;
    }

    return write_count;
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

static void vfs_file_read_forward(struct vnode *node, size_t count, size_t offset)
{
    struct mount_point *mount = node->mount;

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FILE_READ);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);
}

void vfs_file_read(struct vnode *node, size_t count, size_t offset)
{
    struct mount_point *mount = node->mount;
    if (ignore_op_ipc(mount, VFS_OP_FILE_READ)) {
        return;
    }

    rwlock_rlock(&node->rwlock);

    if (node->type == VFS_NODE_PIPE) {
        vfs_pipe_read(node, count);
    } else {
        vfs_file_read_forward(node, count, offset);
    }

    rwlock_runlock(&node->rwlock);
}

static ssize_t vfs_file_write_real(struct vnode *node, const void *data, size_t count, size_t offset)
{
    struct mount_point *mount = node->mount;

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_FILE_WRITE);
    msg_append_param(msg, node->fs_id);
    msg_append_param(msg, count);
    msg_append_param(msg, offset);
    msg_append_data(msg, data, count);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    // Response
    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    size_t write_count = msg_get_param(msg, 1);
    return write_count;
}

ssize_t vfs_file_write(struct vnode *node, const void *data, size_t count, size_t offset)
{
//     struct mount_point *mount = node->mount;
//     if (ignore_op_ipc(mount, VFS_OP_FILE_WRITE)) {
//         return 0;
//     }



    ssize_t write_count = -1;

    // Use read lock here.
    // It's up to the actual FS to determine if multiple writes and reads are
    // allowed concurrently
    rwlock_rlock(&node->rwlock);

    if (node->type == VFS_NODE_PIPE) {
        write_count = vfs_pipe_write(node, data, count);
    } else {
        write_count = vfs_file_write_real(node, data, count, offset);
    }

    rwlock_runlock(&node->rwlock);

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
 * Symlink
 */
void vfs_symlink_read_forward(struct ventry *vent, size_t count)
{
    struct mount_point *mount = vent->from_mount;
    if (ignore_op_ipc(mount, VFS_OP_SYMLINK_READ)) {
        return;
    }

    rwlock_rlock(&vent->rwlock);

    // Request
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, VFS_OP_SYMLINK_READ);
    msg_append_param(msg, vent->fs_id);
    msg_append_param(msg, count);
    msg_append_param(msg, 0);
    syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

    rwlock_runlock(&vent->rwlock);
}

ssize_t vfs_symlink_read_to_buf(struct ventry *vent, char *buf, size_t buf_size)
{
    struct mount_point *mount = vent->from_mount;
    if (ignore_op_ipc(mount, VFS_OP_SYMLINK_READ)) {
        if (buf && buf_size) {
            *buf = '\0';
        }
        return 0;
    }

    // Note that vent should've been wlocked
    //rwlock_rlock(&vent->rwlock);

    int err = 0;
    int more = 1;
    size_t read_count = 0;

    while (buf_size && more) {
        // Request
        msg_t *msg = get_empty_msg();
        msg_append_param(msg, VFS_OP_SYMLINK_READ);
        msg_append_param(msg, vent->fs_id);
        msg_append_param(msg, buf_size);
        msg_append_param(msg, read_count);
        syscall_ipc_popup_request(mount->ipc.pid, mount->ipc.opcode);

        // Response
        msg = get_response_msg();
        err = msg_get_int(msg, 0);
        if (err) {
            break;
        }

        size_t rc = msg_get_param(msg, 1);
        panic_if(rc > buf_size,
                "VFS symlink read returned (%lu) more than requested (%lu)!\n",
                rc, buf_size);

        more = msg_get_int(msg, 2);
        void *read_data = msg_get_data(msg, 3, NULL);
        memcpy(buf, read_data, rc);

        buf_size -= rc;
        buf += rc;

        read_count += rc;
    }

    //rwlock_runlock(&vent->rwlock);

    if (err && buf && buf_size) {
        *buf = '\0';
    }
    return err ? err : read_count;
}


/*
 * IO ops
 */


/*
 * Real path
 */
int vfs_real_path(struct ventry *vent, char *buf, size_t buf_size)
{
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

    // "/"
    if (real_size == 1) {
        if (!buf || buf_size < 2) {
            return -1;
        }

        buf[0] = '/';
        buf[1] = '\0';
        return 0;
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

    //kprintf("pos: %lu, real path: %s\n", pos, buf);
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
