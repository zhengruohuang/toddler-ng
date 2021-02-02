#ifndef __LIBFS_INCLUDE_FS_PSEUDO__
#define __LIBFS_INCLUDE_FS_PSEUDO__


#include <atomic.h>
#include <kth.h>
#include <fs/fs.h>


enum pseudo_fs_data_type {
    PSEUDO_FS_DATA_NONE,
    PSEUDO_FS_DATA_FIXED,
    PSEUDO_FS_DATA_DYNAMIC,
};

struct pseudo_fs_data_block {
    struct pseudo_fs_data_block *next;

    void *data;
    size_t size;
    size_t capacity;
};

struct pseudo_fs_node {
    struct pseudo_fs_node *parent;
    struct pseudo_fs_node *child;
    struct {
        struct pseudo_fs_node *prev;
        struct pseudo_fs_node *next;
    } sibling;

    struct {
        struct pseudo_fs_node *prev;
        struct pseudo_fs_node *next;
    } list;

    fs_id_t id;
    char *name;
    int type;

    int uid;
    int gid;
    unsigned int umask;

    struct {
        int type;

        union {
            struct {
                char *data;
                size_t size;
            } fixed;

            struct {
                struct pseudo_fs_data_block *blocks;
                size_t block_capacity;
            } dyn;
        };
    } data;

    rwlock_t rwlock;
};

struct pseudo_fs {
    struct pseudo_fs_node *root;
    struct pseudo_fs_node *list;

    int read_only;

    rwlock_t rwlock;
    unsigned long id_seq;
};


/*
 * Internal
 */
static inline fs_id_t pseudo_fs_alloc_id(struct pseudo_fs *fs)
{
    unsigned long id = atomic_fetch_and_add(&fs->id_seq, 1);
    if (!id) {
        id = atomic_fetch_and_add(&fs->id_seq, 1);
    }

    return id;
}

extern int pseudo_fs_setup(struct pseudo_fs *fs);
extern int pseudo_fs_node_setup(struct pseudo_fs *fs, struct pseudo_fs_node *node,
                                const char *name, int type, int uid, int gid, unsigned int umask);

extern struct pseudo_fs_node *pseudo_fs_node_find(struct pseudo_fs *fs, fs_id_t id);
extern struct pseudo_fs_node *pseudo_fs_node_lookup(struct pseudo_fs *fs, struct pseudo_fs_node *parent, const char *name);

extern void pseudo_fs_node_attach(struct pseudo_fs *fs, struct pseudo_fs_node *parent, struct pseudo_fs_node *node);
extern void pseudo_fs_node_detach(struct pseudo_fs *fs, struct pseudo_fs_node *parent, struct pseudo_fs_node *node);

extern ssize_t pseudo_fs_set_data(struct pseudo_fs_node *node, int type, void *data, size_t size);
extern ssize_t pseudo_fs_append_data(struct pseudo_fs_node *node, void *data, size_t size);
extern ssize_t pseudo_fs_read_data(struct pseudo_fs_node *node, void *buf, size_t count, size_t offset, int *more);
extern ssize_t pseudo_fs_write_data(struct pseudo_fs_node *node, void *buf, size_t count, size_t offset);

/*
 * Ops
 */
extern int pseudo_fs_dummy(void *fs, fs_id_t id);

extern int pseudo_fs_mount(void *fs, struct fs_mount_config *cfg);
extern int pseudo_fs_unmount(void *fs);

extern int pseudo_fs_lookup(void *fs, fs_id_t id, const char *name, struct fs_lookup_result *result);
extern int pseudo_fs_forget(void *fs, fs_id_t id);

extern int pseudo_fs_acquire(void *fs, fs_id_t id);
extern int pseudo_fs_release(void *fs, fs_id_t id);

extern int pseudo_fs_file_open(void *fs, fs_id_t id, unsigned long flags, unsigned long mode);
extern int pseudo_fs_file_read(void *fs, fs_id_t id, void *buf, size_t count,
                               size_t offset, struct fs_file_op_result *result);
extern int pseudo_fs_file_write(void *fs, fs_id_t id, void *buf, size_t count,
                                size_t offset, struct fs_file_op_result *result);

extern int pseudo_fs_dir_open(void *fs, fs_id_t id, unsigned long flags);
extern int pseudo_fs_dir_read(void *fs, fs_id_t id,
                              fs_id_t last_id, void *last_internal_ptr,
                              struct fs_dir_read_result *result);
extern int pseudo_fs_dir_create(void *fs, fs_id_t id, const char *name, unsigned long flags);
extern int pseudo_fs_dir_remove(void *fs, fs_id_t id);

extern int pseudo_fs_symlink_read(void *fs, fs_id_t id, void *buf, size_t count,
                                  size_t offset, struct fs_file_op_result *result);


/*
 * Create
 */
extern const struct fs_ops pseudo_fs_ops;


#endif
