#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/refcount.h"
#include "libsys/include/syscall.h"
#include "libsys/include/ipc.h"
#include "libk/include/string.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/stdlib.h"
#include "system/include/vfs.h"


/*
 * FS structure
 */
struct root_entry {
    struct root_entry *parent;
    struct root_entry *child;
    struct {
        struct root_entry *prev;
        struct root_entry *next;
    } sibling;

    struct {
        struct root_entry *prev;
        struct root_entry *next;
    } list;

    ulong fs_id;
    ref_count_t ref_count;

    char *name;
    int is_dir;

    char *data;
    size_t size;
};

static volatile ulong fs_id_seq = 1;

static struct root_entry *root = NULL;
static struct root_entry *entries = NULL;

static salloc_obj_t entry_salloc = SALLOC_CREATE(sizeof(struct root_entry), 0, salloc_zero_ctor, NULL);

static inline ulong alloc_fs_id()
{
    return atomic_fetch_and_add(&fs_id_seq, 1);
}

static struct root_entry *find_entry(ulong fs_id)
{
    for (struct root_entry *ent = entries; ent; ent = ent->list.next) {
        if (ent->fs_id == fs_id) {
            return ent;
        }
    }

    return NULL;
}

static struct root_entry *lookup_entry(struct root_entry *parent, const char *name)
{
    for (struct root_entry *ent = parent->child; ent; ent = ent->sibling.next) {
        if (!strcmp(ent->name, name)) {
            return ent;
        }
    }

    return NULL;
}

static char *strdup(const char *s)
{
    char *d = malloc(strlen(s) + 1);
    strcpy(d, s);
    return d;
}

static struct root_entry *new_entry(struct root_entry *parent, const char *name, int dir)
{
    struct root_entry *entry = salloc(&entry_salloc);
    entry->fs_id = alloc_fs_id();
    entry->data = NULL;
    entry->size = 0;
    entry->is_dir = dir;
    entry->name = strdup(name);

    entry->parent = parent;
    entry->child = NULL;
    entry->sibling.prev = NULL;
    if (parent) {
        entry->sibling.next = parent->child;
        if (parent->child) parent->child->sibling.prev = entry;
        parent->child = entry;
    }

    entry->list.prev = NULL;
    entry->list.next = entries;
    entries = entry;

    return entry;
}

static void set_entry_data(struct root_entry *entry, const char *data, size_t size)
{
    entry->data = malloc(size);
    entry->size = size;
    memcpy(entry->data, data, size);
}


/*
 * Response
 */
static inline msg_t *rootfs_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    return msg;
}


/*
 * Lookup
 */
static void rootfs_lookup(msg_t *msg)
{
    // Request
    ulong parent_fs_id = msg_get_param(msg, 1); // ent_fs_id
    char *name = msg_get_data(msg, 2, NULL);    // name

    // Lookup
    struct root_entry *parent = find_entry(parent_fs_id);
    if (!parent) {
        rootfs_response(-1);
        return;
    }

    struct root_entry *entry = lookup_entry(parent, name);
    if (!entry) {
        rootfs_response(-1);
        return;
    }

    //kprintf("RootFS lookup @ %s\n", name);

    // Response
    msg = rootfs_response(0);               // ok
    msg_append_param(msg, entry->fs_id);    // ent_fs_id
    msg_append_int(msg, 1);                 // cacheable
}


/*
 * Acquire
 */
static void rootfs_acquire(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Acquire
    struct root_entry *entry = find_entry(fs_id);
    if (!entry) {
        rootfs_response(-1);
        return;
    }

    ref_count_inc(&entry->ref_count);

    // Response
    msg = rootfs_response(0);               // ok
    msg_append_param(msg, entry->fs_id);    // node_fs_id
    msg_append_int(msg, 1);                 // cacheable
}


/*
 * Release
 */
static void rootfs_release(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Release
    struct root_entry *entry = find_entry(fs_id);
    if (!entry) {
        rootfs_response(-1);
        return;
    }

    ref_count_dec(&entry->ref_count);

    // Response
    msg = rootfs_response(0);               // ok
}


/*
 * File ops
 */
static void rootfs_file_open(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Open
    struct root_entry *entry = find_entry(fs_id);
    if (!entry) {
        rootfs_response(-1);
        return;
    }

    // Response
    msg = rootfs_response(0);               // ok
    msg_append_param(msg, fs_id);           // node_fs_id
}

static void rootfs_file_read(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id
    ulong offset = msg_get_param(msg, 2);   // offset
    ulong size = msg_get_param(msg, 3);     // size

    // Read
    struct root_entry *entry = find_entry(fs_id);
    if (!entry) {
        rootfs_response(-1);
        return;
    }

    kprintf("File read, offset: %lu, size: %lu\n", offset, size);

    // Response
    msg = rootfs_response(0);               // ok

    size_t real_size = 0;
    if (offset < entry->size) {
        real_size = entry->size - offset;
        if (real_size) {
            size_t max_size = msg_remain_data_size(msg) - 0x16ul; // extra safe
            if (real_size > max_size) {
                real_size = max_size;
            }
        }
    }

    msg_append_param(msg, real_size);       // real size
    msg_append_data(msg, entry->data && real_size ? &entry->data[offset] : NULL,
                    real_size);             // data
}


/*
 * Dir ops
 */
static void rootfs_dir_open(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);    // ent_fs_id

    // Open
    struct root_entry *entry = find_entry(fs_id);
    if (!entry || !entry->is_dir) {
        rootfs_response(-1);
        return;
    }

    // Response
    msg = rootfs_response(0);               // ok
}

static void rootfs_dir_read(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);            // ent_fs_id
    ulong start_fs_id = msg_get_param(msg, 2);      // offset
    ulong max_num = msg_get_param(msg, 3);          // size

    // Read
    struct root_entry *entry = find_entry(fs_id);
    if (!entry || !entry->is_dir) {
        rootfs_response(-1);
        return;
    }

    kprintf("Dir read, start: %lu, max num: %lu\n", start_fs_id, max_num);

    // Response
    msg = rootfs_response(0);               // ok
    msg_append_param(msg, 0);               // num entries
    msg_append_param(msg, 0);               // next batch fs_id

    if (entry->child) {
        struct root_entry *child = entry->child;
        if (start_fs_id) {
            child = find_entry(start_fs_id);
            if (!child) {
                return;
            }
        }

        ulong num_entries = 0;
        ulong next_fs_id = 0;
        for (; child; child = child->sibling.next, num_entries++) {
            void *data = msg_try_append_data(msg, NULL, sizeof(ulong) + strlen(entry->name));
            if (data) {
                *(ulong *)data = entry->fs_id;
                data += sizeof(ulong);
                strcmp(data, entry->name);
                next_fs_id = child->sibling.next ? child->sibling.next->fs_id : 0;
            } else {
                next_fs_id = child->fs_id;
                break;
            }
        }

        msg_set_param(msg, 1, num_entries);     // num entries
        msg_set_param(msg, 2, next_fs_id);      // next batch fs_id
    }
}


/*
 * Dispatcher
 */
static ulong rootfs_dispatch(ulong opcode)
{
    msg_t *msg = get_msg();
    ulong vfs_op = msg_get_param(msg, 0);
    kprintf("RootFS op: %lu\n", vfs_op);
    switch (vfs_op) {
    case VFS_OP_LOOKUP:
        rootfs_lookup(msg);
        break;
    case VFS_OP_ACQUIRE:
        rootfs_acquire(msg);
        break;
    case VFS_OP_RELEASE:
        rootfs_release(msg);
        break;
    case VFS_OP_FILE_OPEN:
        rootfs_file_open(msg);
        break;
    case VFS_OP_FILE_READ:
        rootfs_file_read(msg);
        break;
    case VFS_OP_DIR_OPEN:
        rootfs_dir_open(msg);
        break;
    case VFS_OP_DIR_READ:
        rootfs_dir_read(msg);
        break;
    case VFS_OP_DIR_CREATE:
        break;
    case VFS_OP_DIR_REMOVE:
        break;
    default:
        kprintf("Unknown VFS op: %lu\n", vfs_op);
        rootfs_response(-1);
        break;
    }

    syscall_ipc_respond();
    return 0;
}


/*
 * Init
 */
void init_rootfs()
{
    // Root
    root = new_entry(NULL, "root", 1);

    // Subdirs
    new_entry(root, "proc", 1);
    new_entry(root, "sys", 1);
    new_entry(root, "dev", 1);
    new_entry(root, "ipc", 1);

    // File
    const char *about_data = "Welcome!\n";
    struct root_entry *about = new_entry(root, "about", 0);
    set_entry_data(about, about_data, strlen(about_data) + 1);

    // Test
    struct root_entry *test = new_entry(root, "test", 0);
    struct root_entry *test2 = new_entry(test, "test2", 0);
    struct root_entry *test3 = new_entry(test2, "test3", 0);
    struct root_entry *test4 = new_entry(test3, "test4", 0);
    set_entry_data(test4, about_data, strlen(about_data) + 1);

    // Mount
    const int rootfs_opcode = 128;
    const int rootfs_read_only = 0;
    const u32 rootfs_ignored_ops = ~(
        (0x1 << VFS_OP_LOOKUP)      |
        (0x1 << VFS_OP_ACQUIRE)     | (0x1 << VFS_OP_RELEASE)   |
        (0x1 << VFS_OP_FILE_OPEN)   | (0x1 << VFS_OP_FILE_READ) |
        (0x1 << VFS_OP_DIR_OPEN)    | (0x1 << VFS_OP_DIR_READ)  |
        (0x1 << VFS_OP_DIR_CREATE)  | (0x1 << VFS_OP_DIR_REMOVE)
    );

    register_msg_handler(rootfs_opcode, rootfs_dispatch);
    vfs_mount(NULL, "rootfs", syscall_get_tib()->pid, rootfs_opcode,
              root->fs_id, rootfs_read_only, rootfs_ignored_ops);
}
