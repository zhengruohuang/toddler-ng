#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>

#include "common/include/inttypes.h"
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

static salloc_obj_t entry_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct root_entry));

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
    if (entries) {
        entries->list.prev = entry;
    }
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
    ulong req_size = msg_get_param(msg, 2); // size
    ulong offset = msg_get_param(msg, 3);   // offset

    // Read
    struct root_entry *entry = find_entry(fs_id);
    if (!entry) {
        rootfs_response(-1);
        return;
    }

    kprintf("File read, offset: %lu, size: %lu\n", offset, req_size);

    // Response
    msg = rootfs_response(0);               // ok

    size_t real_size = 0;
    if (offset < entry->size) {
        real_size = entry->size - offset;
        if (real_size > req_size) {
            real_size = req_size;
        }

        size_t max_size = msg_remain_data_size(msg) - 0x16ul; // extra safe
        if (real_size > max_size) {
            real_size = max_size;
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
    ulong size = msg_get_param(msg, 2);             // size
    ulong last_fs_id = msg_get_param(msg, 3);       // offset

    // Read
    struct root_entry *entry = find_entry(fs_id);
    if (!entry || !entry->is_dir) {
        rootfs_response(-1);
        return;
    }

    kprintf("Dir read, start: %lu, max bytes: %lu\n", last_fs_id, size);

    // Response
    msg = rootfs_response(0);               // ok
    msg_append_param(msg, 0);               // num entries

    struct root_entry *child = last_fs_id ? find_entry(last_fs_id) : entry->child;
    if (!child || child->parent != entry) {
        return;
    }

    //kprintf("child next @ %p %p\n", child->sibling.next);

    // batch read starts from the entry after ``last_fs_id''
    if (last_fs_id) {
        child = child->sibling.next;
        if (!child) {
            return;
        }
    }

    ulong num_entries = 0;
    size_t copied_size = 0;
    for (; child; child = child->sibling.next, num_entries++) {
        size_t dent_size = sizeof(struct sys_dir_ent) + strlen(child->name) + 1;
        if (copied_size + dent_size > size) {
            break;
        }

        void *buf = msg_try_append_data(msg, NULL, dent_size);
        if (!buf) {
            break;
        }

        struct sys_dir_ent *dent = buf;
        dent->size = dent_size;
        dent->type = 0;
        dent->fs_id = child->fs_id;
        //dent->next_fs_id = child->sibling.next ?
        //                    child->sibling.next->fs_id : 0;
        strcpy(dent->name, child->name);
        //kprintf("copy: %s\n", child->name);

        copied_size += dent_size;
    }

    msg_set_param(msg, 1, num_entries);     // num entries
}

static void rootfs_dir_create(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);            // ent_fs_id
    const char *name = msg_get_data(msg, 2, NULL);  // size
    ulong mode = msg_get_param(msg, 3);             // offset

    // Find entry
    struct root_entry *entry = find_entry(fs_id);
    if (!entry || !entry->is_dir) {
        rootfs_response(-1);
        return;
    }

    // Make sure name is valid
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        rootfs_response(-1);
        return;
    }

    // Make sure the dir hasn't been created
    if (lookup_entry(entry, name)) {
        rootfs_response(-1);
        return;
    }

    // Create
    struct root_entry *new_ent = new_entry(entry, name, 1);

    // Response
    rootfs_response(0);
    msg_append_param(msg, new_ent->fs_id);          // new fs_id
    return;
}

static void rootfs_dir_remove(msg_t *msg)
{
    // Request
    ulong fs_id = msg_get_param(msg, 1);            // ent_fs_id

    // Find entry
    struct root_entry *entry = find_entry(fs_id);
    if (!entry || !entry->is_dir) {
        rootfs_response(-1);
        return;
    }

    // Remove
    // TODO: detach
    struct root_entry *parent_entry = NULL;

    // Response
    rootfs_response(0);
    msg_append_param(msg, parent_entry->fs_id);     // parent fs_id
    return;
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
        rootfs_dir_create(msg);
        break;
    case VFS_OP_DIR_REMOVE:
        rootfs_dir_remove(msg);
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
    const char *about_data =
        " 1 Welcome!\n 2 Welcome!\n 3 Welcome!\n 4 Welcome!\n 5 Welcome!\n"
        " 6 Welcome!\n 7 Welcome!\n 8 Welcome!\n 9 Welcome!\n10 Welcome!\n\n"
        "11 Welcome!\n12 Welcome!\n13 Welcome!\n14 Welcome!\n15 Welcome!\n"
        "16 Welcome!\n17 Welcome!\n18 Welcome!\n19 Welcome!\n20 Welcome!\n\n"
        "21 Welcome!\n22 Welcome!\n23 Welcome!\n24 Welcome!\n25 Welcome!\n"
        "26 Welcome!\n27 Welcome!\n28 Welcome!\n29 Welcome!\n30 Welcome!\n\n"
        "Yeah!\n\n";
    struct root_entry *about = new_entry(root, "about", 0);
    set_entry_data(about, about_data, strlen(about_data) + 1);

    // Test
    struct root_entry *test = new_entry(root, "test", 1);
    struct root_entry *test2 = new_entry(test, "test2", 1);
    struct root_entry *test3 = new_entry(test2, "test3", 1);
    struct root_entry *test4 = new_entry(test3, "test4", 0);
    set_entry_data(test4, about_data, strlen(about_data) + 1);

    new_entry(test, "sub1", 1);
    new_entry(test, "sub2", 1);

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
