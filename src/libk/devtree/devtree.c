#include "common/include/inttypes.h"
#include "common/include/devtree.h"
#include "libk/include/debug.h"
#include "libk/include/string.h"
#include "libk/include/bit.h"


#define DEVTREE_BUF_SIZE 128 * 1024


static char *buf = NULL;   //[DEVTREE_BUF_SIZE];

static int string_offset = 0, struct_offset = 0;
static int string_size = 0, struct_size = 0, remain_size = 0;

static struct devtree_head *head = NULL;
static char *strs = NULL;


/*
 * Tree construction
 */
static void *alloc_struct(int size)
{
    panic_if(!buf, "Buf unallocated!\n");
    panic_if(remain_size < size,
        "Device tree buffer overflow, want: %d, remain_size: %d!\n",
        size, remain_size);

    int prev_struct_offset = struct_offset;
    struct_offset = ALIGN_DOWN(struct_offset - size, sizeof(ulong));

    size = prev_struct_offset - struct_offset;
    struct_size += size;
    remain_size -= size;

    return buf + struct_offset;
}

static void *alloc_and_copy_data(const void *data, int size)
{
    void *dest = alloc_struct(size);
    if (data) {
        memcpy(dest, data, size);
    }

    return dest;
}

static char *alloc_and_copy_str(const char *str)
{
    panic_if(!buf, "Buf unallocated!\n");

    char *exist = memstr(strs, str, string_size);
    if (exist) {
        return exist;
    }

    int size = strlen(str) + 1;
    panic_if(remain_size < size, "Device tree buffer overflow!\n");

    int prev_string_offset = string_offset;
    string_offset = ALIGN_UP(string_offset, sizeof(ulong));
    size += string_offset - prev_string_offset;

    char *dest = buf + string_offset;
    strcpy(dest, str);

    string_offset += size;
    string_size += size;
    remain_size -= size;

    return dest;
}


struct devtree_node *devtree_alloc_node(struct devtree_node *parent,
    const char *name)
{
    struct devtree_node *node = alloc_struct(sizeof(struct devtree_node));

    //kprintf("devtree alloc node: %s @ %p, parent @ %p, remain: %d\n", name,
    //    node, parent, remain_size);

    node->name = DEVTREE_OFFSET(head,
        alloc_and_copy_str(parent ? name : "root"));
    node->prop = 0;
    node->child = 0;

    // Root
    if (!parent) {
        node->parent = 0;
        node->next = 0;
        head->root = DEVTREE_OFFSET(head, node);
    }

    // Regular node
    else {
        node->parent = DEVTREE_OFFSET(head, parent);
        node->next = parent->child;
        parent->child = DEVTREE_OFFSET(head, node);
    }

    return node;
}

struct devtree_prop *devtree_alloc_prop(struct devtree_node *node,
    const char *name, const void *data, int len)
{
    //kprintf("devtree alloc prop: %s, remain: %d\n", name, remain_size);

    struct devtree_prop *prop = alloc_struct(sizeof(struct devtree_prop));

    prop->name = DEVTREE_OFFSET(head, alloc_and_copy_str(name));
    prop->data = len ?
        DEVTREE_OFFSET(head, alloc_and_copy_data(data, len)) : 0;
    prop->len = len;
    prop->parent = DEVTREE_OFFSET(head, node);
    prop->next = node->prop;
    node->prop = DEVTREE_OFFSET(head, prop);

    return prop;
}

struct devtree_prop *devtree_alloc_prop_u32(struct devtree_node *node,
    const char *name, u32 data)
{
    data = swap_big_endian32(data);
    return devtree_alloc_prop(node, name, &data, sizeof(u32));
}

struct devtree_prop *devtree_alloc_prop_u64(struct devtree_node *node,
    const char *name, u64 data)
{
    data = swap_big_endian64(data);
    return devtree_alloc_prop(node, name, &data, sizeof(u64));
}


/*
 * Tree traversal
 *  Note that the tree structure is stored in NATIVE ENDIAN
 */
struct devtree_head *devtree_get_head()
{
    return head;
}

struct devtree_node *devtree_get_root()
{
    if (!head || !head->root) {
        return NULL;
    }

    return DEVTREE_DATA(head, head->root);
}

struct devtree_node *devtree_get_next_node(struct devtree_node *node)
{
    if (!node) {
        return devtree_get_root();
    }

    return DEVTREE_DATA(head, node->next);
}

struct devtree_node *devtree_get_child_node(struct devtree_node *node)
{
    if (!node) {
        return devtree_get_root();
    }

    return DEVTREE_DATA(head, node->child);
}

struct devtree_node *devtree_get_parent_node(struct devtree_node *node)
{
    return DEVTREE_DATA(head, node->parent);
}

struct devtree_prop *devtree_get_prop(struct devtree_node *node)
{
    return DEVTREE_DATA(head, node->prop);
}

struct devtree_prop *devtree_get_next_prop(struct devtree_prop *prop)
{
    return DEVTREE_DATA(head, prop->next);
}


/*
 * Value
 *  Note that all values are stored in BIG ENDIAN unless explicitly swaps
 */
void *devtree_get_data(int offset)
{
    return DEVTREE_DATA(head, offset);
}

char *devtree_get_node_name(struct devtree_node *node)
{
    return DEVTREE_DATA(head, node->name);
}

char *devtree_get_prop_name(struct devtree_prop *prop)
{
    return DEVTREE_DATA(head, prop->name);
}

void *devtree_get_prop_data(struct devtree_prop *prop)
{
    return DEVTREE_DATA(head, prop->data);
}

u32 devtree_get_prop_data_u32(struct devtree_prop *prop)
{
    panic_if(prop->len != sizeof(u32),
        "Prop len: %d, expects: %d\n", prop->len, 4);

    u32 *data = DEVTREE_DATA(head, prop->data);
    return swap_big_endian32(*data);
}

u64 devtree_get_prop_data_u64(struct devtree_prop *prop)
{
    panic_if(prop->len != sizeof(u64),
        "Prop len: %d, expects: %d\n", prop->len, 8);

    u64 *data = DEVTREE_DATA(head, prop->data);
    return swap_big_endian64(*data);
}


/*
 * Init
 */
void open_libk_devtree(struct devtree_head *devtree)
{
    head = devtree;
}

struct devtree_head *create_libk_devtree(void *_buf, ulong size)
{
    buf = _buf;
    memzero(buf, size);

    head = (void *)buf;
    strs = buf + sizeof(struct devtree_head);
    head->strs = DEVTREE_OFFSET(head, strs);
    head->buf_size = size;

    string_offset = sizeof(struct devtree_head);
    struct_offset = DEVTREE_BUF_SIZE;

    string_size = 0;
    struct_size = 0;
    remain_size = DEVTREE_BUF_SIZE - sizeof(struct devtree_head);

    return head;
}

size_t devtree_get_buf_size()
{
    return head->buf_size;
}

size_t devtree_get_buf_size2(struct devtree_head *dt)
{
    return dt->buf_size;
}
