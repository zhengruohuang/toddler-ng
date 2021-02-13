#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/api.h>
#include <fs/pseudo.h>


/*
 * Internal structure
 */
int pseudo_fs_setup(struct pseudo_fs *fs)
{
    memzero(fs, sizeof(struct pseudo_fs));

    fs->id_seq = 1;
    fs->list = NULL;
    fs->root = NULL;

    return 0;
}

int pseudo_fs_node_setup(struct pseudo_fs *fs, struct pseudo_fs_node *node,
                         const char *name, int type, int uid, int gid, unsigned int umask)
{
    memzero(node, sizeof(struct pseudo_fs_node));

    node->id = pseudo_fs_alloc_id(fs);
    node->name = strdup(name);
    node->type = type;

    node->uid = uid;
    node->gid = gid;
    node->umask = umask;

    node->data.type = PSEUDO_FS_DATA_NONE;

    // TODO: time

    return 0;
}


struct pseudo_fs_node *pseudo_fs_node_find(struct pseudo_fs *fs, fs_id_t id)
{
    for (struct pseudo_fs_node *node = fs->list; node; node = node->list.next) {
        if (node->id == id) {
            return node;
        }
    }

    return NULL;
}

struct pseudo_fs_node *pseudo_fs_node_lookup(struct pseudo_fs *fs, struct pseudo_fs_node *parent, const char *name)
{
    for (struct pseudo_fs_node *node = parent->child; node; node = node->sibling.next) {
        if (!strcmp(node->name, name)) {
            return node;
        }
    }

    return NULL;
}


void pseudo_fs_node_attach(struct pseudo_fs *fs, struct pseudo_fs_node *parent, struct pseudo_fs_node *node)
{
    node->parent = parent;
    node->child = NULL;
    node->sibling.prev = NULL;
    if (parent) {
        node->sibling.next = parent->child;
        if (parent->child) parent->child->sibling.prev = node;
        parent->child = node;
    } else {
        fs->root = node;
    }

    node->list.prev = NULL;
    node->list.next = fs->list;
    if (fs->list) {
        fs->list->list.prev = node;
    }
    fs->list = node;
}

void pseudo_fs_node_detach(struct pseudo_fs *fs, struct pseudo_fs_node *parent, struct pseudo_fs_node *node)
{
    if (node->sibling.prev) {
        node->sibling.prev->sibling.next = node->sibling.next;
    }
    if (node->sibling.next) {
        node->sibling.next->sibling.prev = node->sibling.prev;
    }
    if (parent->child == node) {
        parent->child = node->sibling.next;
    }

    if (node->list.prev) {
        node->list.prev->list.next = node->list.next;
    }
    if (node->list.next) {
        node->list.next->list.prev = node->list.prev;
    }
    if (fs->list == node) {
        fs->list = node->list.next;
    }
}


ssize_t pseudo_fs_set_data(struct pseudo_fs_node *node, int type, void *data, size_t size)
{
    ssize_t ret = -1;
    rwlock_wlock(&node->rwlock);

    if (node->data.type == PSEUDO_FS_DATA_NONE) {
        switch (type) {
        case PSEUDO_FS_DATA_FIXED:
            node->data.type = PSEUDO_FS_DATA_FIXED;
            node->data.fixed.size = size;
            node->data.fixed.data = data;
            //node->data.fixed.data = malloc(size);
            //memcpy(node->data.fixed.data, data, size);
            break;
        case PSEUDO_FS_DATA_DYNAMIC:
            node->data.type = PSEUDO_FS_DATA_DYNAMIC;
            node->data.dyn.blocks = NULL;
            node->data.dyn.block_capacity = size;
            if (data) {
                pseudo_fs_write_data(node, data, size, 0);
            }
            break;
        default:
            break;
        }
    }

    rwlock_wunlock(&node->rwlock);
    return ret;
}

ssize_t pseudo_fs_append_data(struct pseudo_fs_node *node, void *data, size_t size)
{
    return 0;
}

ssize_t pseudo_fs_read_data(struct pseudo_fs_node *node, void *buf, size_t count, size_t offset, int *more)
{
    ssize_t read_size = 0;
    rwlock_rlock(&node->rwlock);

    switch (node->data.type) {
    case PSEUDO_FS_DATA_FIXED:
        if (offset < node->data.fixed.size && count) {
            read_size = node->data.fixed.size - offset;
            if (read_size > count) {
                read_size = count;
                *more = 1;
            } else {
                *more = 0;
            }
            memcpy(buf, node->data.fixed.data + offset, read_size);
        }
        break;
    case PSEUDO_FS_DATA_DYNAMIC:
        break;
    default:
        break;
    }

    rwlock_runlock(&node->rwlock);
    return read_size;
}

ssize_t pseudo_fs_write_data(struct pseudo_fs_node *node, void *buf, size_t count, size_t offset)
{
    return 0;
}


/*
 * Operations
 */
static inline int _is_file(struct pseudo_fs_node *node)
{
    return node->type == VFS_NODE_FILE;
}

static inline int _is_dir(struct pseudo_fs_node *node)
{
    return node->type == VFS_NODE_DIR;
}

static inline int _is_symlink(struct pseudo_fs_node *node)
{
    return node->type == VFS_NODE_SYMLINK;
}

static inline int _is_dev(struct pseudo_fs_node *node)
{
    return node->type == VFS_NODE_DEV;
}

static inline int _is_pipe(struct pseudo_fs_node *node)
{
    return node->type == VFS_NODE_PIPE;
}

int pseudo_fs_mount(void *fs, struct fs_mount_config *cfg)
{
    struct pseudo_fs *pfs = fs;
    cfg->read_only = pfs->read_only;
    cfg->root_id = pfs->root ? pfs->root->id : 0;

    return 0;
}

int pseudo_fs_unmount(void *fs)
{
    return 0;
}

int pseudo_fs_dummy(void *fs, fs_id_t id)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *node = pseudo_fs_node_find(fs, id);
    err = node ? 0 : -1;

    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_lookup(void *fs, fs_id_t id, const char *name, struct fs_lookup_result *result)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *parent = pseudo_fs_node_find(fs, id);
    if (!parent) {
        err = -1;
        goto done;
    }

    struct pseudo_fs_node *node = pseudo_fs_node_lookup(fs, parent, name);
    if (!node) {
        err = -1;
        goto done;
    }

    result->id = node->id;
    result->cacheable = 1;
    result->node_type = node->type;

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_forget(void *fs, fs_id_t id)
{
    return pseudo_fs_dummy(fs, id);
}

int pseudo_fs_acquire(void *fs, fs_id_t id, struct fs_lookup_result *result)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *node = pseudo_fs_node_find(fs, id);
    if (!node) {
        err = -1;
        goto done;
    }

    result->id = node->id;
    result->cacheable = 1;
    result->node_type = node->type;

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_release(void *fs, fs_id_t id)
{
    return pseudo_fs_dummy(fs, id);
}

int pseudo_fs_file_open(void *fs, fs_id_t id, unsigned long flags, unsigned long mode)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *node = pseudo_fs_node_find(fs, id);
    if (!node || (!_is_file(node) && !_is_pipe(node) && !_is_dev(node))) {
        err = -1;
        goto done;
    }

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_file_read(void *fs, fs_id_t id, void *buf, size_t count,
                        size_t offset, struct fs_file_op_result *result)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *node = pseudo_fs_node_find(fs, id);
    if (!node || !_is_file(node)) {
        err = -1;
        goto done;
    }

    result->more = 0;
    result->count = pseudo_fs_read_data(node, buf, count, offset, &result->more);
    if (result->count < 0) {
        err = result->count;
        goto done;
    }

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_file_write(void *fs, fs_id_t id, void *buf, size_t count,
                         size_t offset, struct fs_file_op_result *result)
{
    return -1;
}

int pseudo_fs_dir_open(void *fs, fs_id_t id, unsigned long flags)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *node = pseudo_fs_node_find(fs, id);
    if (!node || !_is_dir(node)) {
        err = -1;
        goto done;
    }

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_dir_read(void *fs, fs_id_t id,
                       fs_id_t last_id, void *last_internal_ptr,
                       struct fs_dir_read_result *result)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *parent = pseudo_fs_node_find(fs, id);
    if (!parent || !_is_dir(parent)) {
        err = -1;
        goto done;
    }

    struct pseudo_fs_node *node = NULL;
    if (last_internal_ptr) {
        node = last_internal_ptr;
        node = node->sibling.next;
    } else {
        if (last_id) {
            node = pseudo_fs_node_find(fs, last_id);
            if (node) {
                node = node->sibling.next;
            }
        } else {
            node = parent->child;
        }
    }

    result->internal_ptr = node;
    if (node) {
        result->id = node->id;
        result->type = node->type;
        result->name = node->name;
        err = 1;
    } else {
        result->id = 0;
    }

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_dir_create(void *fs, fs_id_t id, const char *name, unsigned long flags)
{
    return -1;
}

int pseudo_fs_dir_remove(void *fs, fs_id_t id)
{
    return -1;
}

int pseudo_fs_symlink_read(void *fs, fs_id_t id, void *buf, size_t count,
                           size_t offset, struct fs_file_op_result *result)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    rwlock_rlock(&pfs->rwlock);

    struct pseudo_fs_node *node = pseudo_fs_node_find(fs, id);
    if (!node || !_is_symlink(node)) {
        err = -1;
        goto done;
    }

    result->more = 0;
    result->count = pseudo_fs_read_data(node, buf, count, offset, &result->more);
    if (result->count < 0) {
        err = result->count;
        goto done;
    }

done:
    rwlock_runlock(&pfs->rwlock);
    return err;
}

int pseudo_fs_pipe_create(void *fs, fs_id_t id, const char *name, unsigned long flags)
{
    int err = 0;
    struct pseudo_fs *pfs = fs;
    if (!pfs->ops.alloc_node_pipe) {
        return -1;
    }

    rwlock_wlock(&pfs->rwlock);

    struct pseudo_fs_node *parent = pseudo_fs_node_find(fs, id);
    if (!parent || !_is_dir(parent)) {
        err = -1;
        goto done;
    }

    struct pseudo_fs_node *node = pseudo_fs_node_lookup(fs, parent, name);
    if (node) {
        err = -1;
        goto done;
    }

    struct pseudo_fs_node *fs_node = pfs->ops.alloc_node_pipe();
    pseudo_fs_node_setup(pfs, fs_node, name, VFS_NODE_PIPE, 0, 0, 0);
    pseudo_fs_node_attach(pfs, parent, fs_node);

done:
    rwlock_wunlock(&pfs->rwlock);
    return err;
}


/*
 * Create
 */
const struct fs_ops pseudo_fs_ops = {
    .mount = pseudo_fs_mount,
    .unmount = pseudo_fs_unmount,

    .lookup = pseudo_fs_lookup,
    .forget = pseudo_fs_forget,

    .acquire = pseudo_fs_acquire,
    .release = pseudo_fs_release,

    .file_open = pseudo_fs_file_open,
    .file_read = pseudo_fs_file_read,
    .file_write = pseudo_fs_file_write,

    .dir_open = pseudo_fs_dir_open,
    .dir_read = pseudo_fs_dir_read,
    .dir_create = pseudo_fs_dir_create,
    .dir_remove = pseudo_fs_dir_remove,

    .symlink_read = pseudo_fs_symlink_read,

    .pipe_create = pseudo_fs_pipe_create,
};
