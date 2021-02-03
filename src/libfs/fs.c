#include <atomic.h>
#include <string.h>
#include <assert.h>
#include <sys.h>
#include <sys/api.h>
#include <fs/fs.h>


#define FS_IPC_OPCODE_BASE  128

struct fs_record {
    void *fs;
    struct fs_ops ops;

    const char *path;
    const char *name;

    unsigned long ignored_ops_map;
    int read_only;
};


/*
 * FS dispatch index
 */
// Maximum number of file systems a process can register
#define MAX_NUM_FS  8

static unsigned long fs_idx = 0;
static struct fs_record fs_records[MAX_NUM_FS] = { };

static unsigned long alloc_fs_idx()
{
    unsigned long idx = atomic_fetch_and_add(&fs_idx, 1);
    panic_if(idx >= MAX_NUM_FS, "Too many file systems in a process!\n");
    return idx;
}


/*
 * Response
 */
static inline msg_t *dispatch_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    return msg;
}


/*
 * Mount
 */
static inline void dispatch_mount(msg_t *msg, struct fs_record *record)
{
    // Request
    // Empty param

    // Mount
    int err = 0;
    struct fs_mount_config cfg;
    memzero(&cfg, sizeof(struct fs_mount_config));
    if (record->ops.mount) {
        err = record->ops.mount(record->fs, &cfg);
    }

    // Save
    record->read_only = cfg.read_only;

    // Response
    msg = dispatch_response(err);
    msg_append_param(msg, cfg.root_id);
    msg_append_int(msg, cfg.read_only);
}

static inline void dispatch_unmount(msg_t *msg, struct fs_record *record)
{
    // Request
    // Empty param

    // Unmount
    int err = 0;
    if (record->ops.unmount) {
        err = record->ops.unmount(record->fs);
    }

    // Response
    msg = dispatch_response(err);
}


/*
 * Lookup
 */
static inline void dispatch_lookup(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong parent_fs_id = msg_get_param(msg, 1); // ent_fs_id
    char *name = msg_get_data(msg, 2, NULL);    // name

    //kprintf("lookup: %s\n", name);

    // Lookup
    int err = 0;
    struct fs_lookup_result result = { };
    if (record->ops.lookup) {
        err = record->ops.lookup(record->fs, parent_fs_id, name, &result);
    }

    //kprintf("id: %lu, err: %d\n", result.id, err);

    // Response
    msg = dispatch_response(err);
    msg_append_param(msg, result.id);       // ent_fs_id
    msg_append_int(msg, result.cacheable);  // cacheable
    msg_append_int(msg, result.node_type);  // node type
}


/*
 * Acquire
 */
static inline void dispatch_acquire(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);        // ent_fs_id

    // Acquire
    int err = 0;
    struct fs_lookup_result result = { };
    if (record->ops.acquire) {
        err = record->ops.acquire(record->fs, fs_id, &result);
    }

    // Response
    msg = dispatch_response(err);
    msg_append_param(msg, fs_id);
    msg_append_int(msg, result.cacheable);      // TODO: cacheable, necessary?
    msg_append_int(msg, result.node_type);      // Node type
}

static inline void dispatch_release(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Release
    int err = 0;
    if (record->ops.release) {
        err = record->ops.release(record->fs, fs_id);
    }

    // Response
    msg = dispatch_response(err);
}


/*
 * Special node
 */
static inline void dispatch_dev_create(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);            // ent_fs_id
    const char *name = msg_get_data(msg, 2, NULL);  // name
    ulong flags = msg_get_param(msg, 3);            // flags
    pid_t pid = msg_get_param(msg, 4);              // pid
    unsigned long opcode = msg_get_param(msg, 5);   // opcode

    // Open
    int err = 0;
    if (record->ops.dev_create) {
        err = record->ops.dev_create(record->fs, fs_id, name, flags, pid, opcode);
    }

    // Response
    msg = dispatch_response(err);
}

static inline void dispatch_pipe_create(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);            // ent_fs_id
    const char *name = msg_get_data(msg, 2, NULL);  // name
    ulong flags = msg_get_param(msg, 3);            // flags

    // Open
    int err = 0;
    if (record->ops.pipe_create) {
        err = record->ops.pipe_create(record->fs, fs_id, name, flags);
    }

    // Response
    msg = dispatch_response(err);
}


/*
 * File ops
 */
static inline void dispatch_file_open(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong flags = msg_get_param(msg, 2);    // flags
    ulong mode = msg_get_param(msg, 3);     // mode

    // Open
    int err = 0;
    if (record->ops.file_open) {
        err = record->ops.file_open(record->fs, fs_id, flags, mode);
    }

    // Response
    msg = dispatch_response(err);
}

static inline void dispatch_file_read(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset

    // Response
    msg = dispatch_response(0);             // ok
    msg_append_param(msg, 0);               // real size
    msg_append_int(msg, 0);                 // more

    // Buffer
    size_t max_size = msg_remain_data_size(msg) - 32ul; // extra safe
    if (req_size > max_size) req_size = max_size;
    void *buf = msg_append_data(msg, NULL, req_size);   // data

    //kprintf("FS read, req_size: %lu, offset: %lu\n", req_size, offset);

    // Read
    int err = 0;
    if (record->ops.file_read) {
        struct fs_file_op_result result = { };
        err = record->ops.file_read(record->fs, fs_id, buf, req_size, offset, &result);

        if (!err) {
            msg_set_param(msg, 1, result.count);
            msg_set_int(msg, 2, result.more);
        }
    }

    // Response
    msg_set_int(msg, 0, err);
}

static inline void dispatch_file_write(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset
    void *data = msg_get_data(msg, 4, NULL);// data

    //kprintf("FS write, req_size: %lu, offset: %lu\n", req_size, offset);

    // Read
    int err = 0;
    size_t write_size = 0;
    if (record->ops.file_write) {
        struct fs_file_op_result result = { };
        err = record->ops.file_write(record->fs, fs_id, data, req_size, offset, &result);

        if (!err) {
            write_size = result.count;
        }
    }

    // Response
    msg = dispatch_response(err);           // err
    if (!err) {
        msg_append_param(msg, write_size);  // real size
    }
}


/*
 * Dir ops
 */
static inline void dispatch_dir_open(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong flags = msg_get_param(msg, 2);    // flags

    // Open
    int err = 0;
    if (record->ops.dir_open) {
        err = record->ops.dir_open(record->fs, fs_id, flags);
    }

    // Response
    msg = dispatch_response(err);
}

static inline void dispatch_dir_read(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong parent_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong size = msg_get_param(msg, 2);         // size
    ulong last_id = msg_get_param(msg, 3);      // offset

    // Response
    msg = dispatch_response(0);                 // ok
    msg_append_param(msg, 0);                   // num entries

    // Read
    int err = 0;
    ulong num_entries = 0;

    if (record->ops.dir_read) {
        size_t copied_size = 0;
        struct fs_dir_read_result result = { };

        err = record->ops.dir_read(record->fs, parent_id, last_id, NULL, &result);
        while (err == 1) {
            //kprintf("name: %s\n", result.name);

            size_t dent_size = sizeof(struct sys_dir_ent) + strlen(result.name) + 1;
            if (copied_size + dent_size > size) {
                break;
            }

            void *buf = msg_try_append_data(msg, NULL, dent_size);
            if (!buf) {
                break;
            }

            struct sys_dir_ent *dent = buf;
            dent->size = dent_size;
            dent->type = result.type;
            dent->fs_id = result.id;
            strcpy(dent->name, result.name);
            //kprintf("copy: %s\n", result.name);

            copied_size += dent_size;
            num_entries++;

            err = record->ops.dir_read(record->fs, parent_id, result.id, result.internal_ptr, &result);
        }
    }

    // Response
    msg_set_int(msg, 0, err < 0 ? err : 0);
    msg_set_param(msg, 1, num_entries);
}


/*
 * Symlink ops
 */
static inline void dispatch_symlink_read(msg_t *msg, struct fs_record *record)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset

    // Response
    msg = dispatch_response(0);             // ok
    msg_append_param(msg, 0);               // real size
    msg_append_int(msg, 0);                 // more

    // Buffer
    size_t max_size = msg_remain_data_size(msg) - 32ul; // extra safe
    if (req_size > max_size) req_size = max_size;
    void *buf = msg_append_data(msg, NULL, req_size);   // data

    //kprintf("FS symlink read, req_size: %lu\n", req_size);

    // Read
    int err = 0;
    if (record->ops.symlink_read) {
        struct fs_file_op_result result = { };
        err = record->ops.symlink_read(record->fs, fs_id, buf, req_size, offset, &result);

        if (!err) {
            msg_set_param(msg, 1, result.count);
            msg_set_int(msg, 2, result.more);
        }
    }

    // Response
    msg_set_int(msg, 0, err);
}


/*
 * Dispatch
 */
static unsigned long dispatch(unsigned long opcode)
{
    panic_if(opcode < FS_IPC_OPCODE_BASE ||
             opcode >= FS_IPC_OPCODE_BASE + MAX_NUM_FS,
             "Invalid FS opcode!\n");

    int idx = opcode - FS_IPC_OPCODE_BASE;
    struct fs_record *record = &fs_records[idx];

    msg_t *msg = get_msg();
    ulong vfs_op = msg_get_param(msg, 0);
    //kprintf("%s op: %lu\n", record->name, vfs_op);

    switch (vfs_op) {
    case VFS_OP_MOUNT:
        dispatch_mount(msg, record);
        break;
    case VFS_OP_UNMOUNT:
        dispatch_unmount(msg, record);
        break;
    case VFS_OP_LOOKUP:
        dispatch_lookup(msg, record);
        break;
    case VFS_OP_FORGET:
        dispatch_response(0); // TODO
        break;
    case VFS_OP_ACQUIRE:
        dispatch_acquire(msg, record);
        break;
    case VFS_OP_RELEASE:
        dispatch_release(msg, record);
        break;
    case VFS_OP_DEV_CREATE:
        dispatch_dev_create(msg, record);
        break;
    case VFS_OP_PIPE_CREATE:
        dispatch_pipe_create(msg, record);
        break;
    case VFS_OP_FILE_OPEN:
        dispatch_file_open(msg, record);
        break;
    case VFS_OP_FILE_READ:
        dispatch_file_read(msg, record);
        break;
    case VFS_OP_FILE_WRITE:
        dispatch_file_write(msg, record);
        break;
    case VFS_OP_DIR_OPEN:
        dispatch_dir_open(msg, record);
        break;
    case VFS_OP_DIR_READ:
        dispatch_dir_read(msg, record);
        break;
    case VFS_OP_DIR_CREATE:
        //dispatch_dir_create(msg, record);
        break;
    case VFS_OP_DIR_REMOVE:
        //dispatch_dir_remove(msg, record);
        break;
    case VFS_OP_SYMLINK_READ:
        dispatch_symlink_read(msg, record);
        break;
    default:
        kprintf("Unknown VFS op: %lu in %s\n", vfs_op, record->name);
        dispatch_response(-1);
        break;
    }

    syscall_ipc_respond();
    return 0;
}


/*
 * Register and mount a new file system
 */
static inline unsigned long get_ignored_ops_map(const struct fs_ops *ops)
{
    unsigned long mask = 0;

    if (!ops || !ops->mount)    mask |= 0x1 << VFS_OP_MOUNT;
    if (!ops || !ops->unmount)  mask |= 0x1 << VFS_OP_UNMOUNT;

    if (!ops || !ops->lookup)   mask |= 0x1 << VFS_OP_LOOKUP;
    if (!ops || !ops->forget)   mask |= 0x1 << VFS_OP_FORGET;

    if (!ops || !ops->acquire)  mask |= 0x1 << VFS_OP_ACQUIRE;
    if (!ops || !ops->release)  mask |= 0x1 << VFS_OP_RELEASE;

    if (!ops || !ops->dev_create)   mask |= 0x1 << VFS_OP_DEV_CREATE;
    if (!ops || !ops->pipe_create)  mask |= 0x1 << VFS_OP_PIPE_CREATE;

    if (!ops || !ops->file_open)    mask |= 0x1 << VFS_OP_FILE_OPEN;
    if (!ops || !ops->file_read)    mask |= 0x1 << VFS_OP_FILE_READ;
    if (!ops || !ops->file_write)   mask |= 0x1 << VFS_OP_FILE_WRITE;

    if (!ops || !ops->dir_open)     mask |= 0x1 << VFS_OP_DIR_OPEN;
    if (!ops || !ops->dir_read)     mask |= 0x1 << VFS_OP_DIR_READ;
    if (!ops || !ops->dir_create)   mask |= 0x1 << VFS_OP_DIR_CREATE;
    if (!ops || !ops->dir_remove)   mask |= 0x1 << VFS_OP_DIR_REMOVE;

    if (!ops || !ops->symlink_read) mask |= 0x1 << VFS_OP_SYMLINK_READ;

    return mask;
}

int create_fs(const char *path, const char *name, void *fs, const struct fs_ops *ops, int popup)
{
    unsigned long idx = alloc_fs_idx();
    unsigned long opcode = FS_IPC_OPCODE_BASE + idx;

    struct fs_record *record = &fs_records[idx];
    memzero(record, sizeof(struct fs_record));
    record->fs = fs;
    record->path = strdup(path ? path : "/");
    record->name = strdup(name);
    record->ignored_ops_map = get_ignored_ops_map(ops);
    memcpy(&record->ops, ops, sizeof(struct fs_ops));

    if (popup) {
        register_msg_handler(opcode, dispatch);
    } else {
        // TODO: serial handler
    }

    int fd = path ? sys_api_acquire(path) : 0;
    panic_if(path && fd <= 0, "Unable to mount FS!\n");

    sys_api_mount(fd, name, opcode, record->ignored_ops_map);

    return 0;
}
