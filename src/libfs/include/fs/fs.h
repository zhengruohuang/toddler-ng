#ifndef __LIBFS_INCLUDE_FS_H__
#define __LIBFS_INCLUDE_FS_H__


#include <sys.h>
#include <fs/op.h>


/*
 * Operations
 */
typedef unsigned long fs_id_t;

struct fs_mount_config {
    int read_only;
    fs_id_t root_id;
};

struct fs_lookup_result {
    fs_id_t id;
    int cacheable;
    int node_type;
};

struct fs_file_op_result {
    ssize_t count;
    int more;
};

struct fs_dir_read_result {
    void *internal_ptr;
    fs_id_t id;
    int type;
    char *name;
};

struct fs_ops {
    int (*mount)(void *fs, struct fs_mount_config *cfg);
    int (*unmount)(void *fs);

    int (*lookup)(void *fs, fs_id_t id, const char *name, struct fs_lookup_result *result);
    int (*forget)(void *fs, fs_id_t id);

    int (*acquire)(void *fs, fs_id_t id);
    int (*release)(void *fs, fs_id_t id);

    int (*dev_create)(void *fs, fs_id_t id, const char *name, unsigned long flags, pid_t pid, unsigned opcode);
    int (*pipe_create)(void *fs, fs_id_t id, const char *name, unsigned long flags);

    int (*file_open)(void *fs, fs_id_t id, unsigned long flags, unsigned long mode);
    int (*file_read)(void *fs, fs_id_t id, void *buf, size_t count, size_t offset,
                     struct fs_file_op_result *result);
    int (*file_write)(void *fs, fs_id_t id, void *buf, size_t count, size_t offset,
                      struct fs_file_op_result *result);

    int (*dir_open)(void *fs, fs_id_t id, unsigned long flags);
    int (*dir_read)(void *fs, fs_id_t id, fs_id_t last_id, void *last_internal_ptr,
                    struct fs_dir_read_result *result);
    int (*dir_create)(void *fs, fs_id_t id, const char *name, unsigned long flags);
    int (*dir_remove)(void *fs, fs_id_t id);

    int (*symlink_read)(void *fs, fs_id_t id, void *buf, size_t count, size_t offset,
                        struct fs_file_op_result *result);
};


extern int create_fs(const char *path, const char *name, void *fs, const struct fs_ops *ops, int popup);


#endif
