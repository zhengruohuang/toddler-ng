#include "common/include/arch.h"
#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/obp.h"
#include "loader/include/devtree.h"
#include "loader/include/kprintf.h"
#include "loader/include/firmware.h"


#define MAX_OBP_BUF_LEN     512
#define MAX_OBP_NAME_LEN    32


static struct openboot_prom *obp = NULL;

static int obp_root = 0;
static int obp_chosen = 0;


/*
 * Device operations
 */
static int obp_open(const char *name)
{
    return obp->dev_ops_v2.dev_open(name);
}

static void obp_close(int dev)
{
    obp->dev_ops_v2.dev_close(dev);
}

static int inst2pkg(int dev)
{
    return obp->dev_ops_v2.inst2pkg(dev);
}


/*
 * Node operations
 */
static char obp_buf[MAX_OBP_BUF_LEN];

static int get_prop(int node, const char *name, void *buf, int len)
{
    int ret = obp->node_ops->get_prop(node, name, obp_buf);

    int copied = 0;
    if (ret > 0 && len > 0 && buf) {
        copied = ret > len ? len : ret;
        memcpy(buf, obp_buf, copied);
    }

    return copied;
}

static const char *get_next_prop(int node, const char *name)
{
    return obp->node_ops->next_prop(node, name);
}

static int get_prop_len(int node, const char *name)
{
    return obp->node_ops->prop_len(node, name);
}

static int get_peer_node(int node)
{
    return obp->node_ops->next_node(node);
}

static int get_child_node(int node)
{
    return obp->node_ops->child(node);
}

static int get_node_name(int node, char *buf, int buf_len)
{
    if (node == obp_root) {
        strcpy(buf, "/");
        return 1;
    }

//     kprintf("Here?\n");

    int name_len = (int)get_prop_len(node, "name");
    get_prop(node, "name", buf, buf_len);

    int reg_len = (int)get_prop_len(node, "reg");
    if (reg_len >= 2) {
        if (name_len) {
            buf[name_len - 1] = '@';
            buf[name_len++] = '\0';
        } else {
            buf[0] = '@';
            buf[1] = '\0';
            buf_len = 2;
        }

        u32 reg[8];
        get_prop(node, "reg", reg, sizeof(reg));

        int started = 0;

        u32 r = reg[0];
        for (int i = 0; i < 32; i += 4) {
            int d = (r >> (28 - i)) & 0xf;
            if (started || d) {
                started = 1;
                buf[name_len - 1] = d >= 10 ? 'a' + d - 10 : '0' + d;
                buf[name_len++] = '\0';
            }
        }

        if (reg_len >= 3) {
            r = reg[1];
            for (int i = 0; i < 32; i += 4) {
                int d = (r >> (28 - i)) & 0xf;
                if (started || d) {
                    started = 1;
                    buf[name_len - 1] = d >= 10 ? 'a' + d - 10 : '0' + d;
                    buf[name_len++] = '\0';
                }
            }
        }

        if (!started) {
            buf[name_len - 1] = '0';
            buf[name_len++] = '\0';
        }
    }

    kprintf("|%s|\n", buf);

    // FIXME
    return name_len;
}


/*
 * Copy device tree
 */
static char node_name[MAX_OBP_NAME_LEN];

static void copy_node(int handle, struct devtree_node *parent)
{
    // Process all nodes in the same level
    for (int i = 0; handle; handle = get_peer_node(handle), i++) {
        kprintf("Handle: %d, peer: %d, child: %d\n", handle, get_peer_node(handle), get_child_node(handle));

        // Root should not have a peer
        if (!parent) {
            panic_if(i > 0, "OFW root node must not have a peer!\n");
        }

        // Get the disambigued name
        int len = get_node_name(handle, node_name, MAX_OBP_BUF_LEN);
        if (len <= 0) {
            continue;
        }

        // Create the devtree node
        kprintf("Copy node: |%s|\n", node_name);
        struct devtree_node *node = devtree_alloc_node(parent, node_name);

        // Find and copy props
        for (const char *name = get_next_prop(handle, NULL); name && name[0];
            name = get_next_prop(handle, name)
        ) {
            int data_len = (int)get_prop_len(handle, name);
            kprintf("\tCopy prop: %s, len: %d\n", name, data_len);

            struct devtree_prop *prop = devtree_alloc_prop(node, name, NULL, data_len);
            if (data_len) {
                get_prop(handle, name, devtree_get_prop_data(prop), data_len);
            }
        }

        // Go to child node
        int child = get_child_node(handle);
        if (child) {
            kprintf("Next level\n");
            copy_node(child, node);
        }
    }

    kprintf("Done\n");
}

static void copy_devtree()
{
    copy_node(get_child_node(obp_root), NULL);
}


/*
 * Init
 */
static void init_obp_nodes()
{
    obp_root = obp_open("/");
    panic_if(obp_root == -1, "OBP unable to find path /");

    obp_chosen = obp_open("/chosen");
    panic_if(obp_chosen == -1, "OBP unable to find path /chosen");

//     int ret = (int)get_prop(ofw_chosen, "mmu", &ofw_mmu, sizeof(ofw_mmu));
//     panic_if(ret <= 0, "OFW unable to find MMU");
}

void init_obp(void *entry)
{
    obp = entry;
    init_obp_nodes();
    copy_devtree();
}


/*
 * Memory allocation and mapping
 */
paddr_t obp_translate_virt_to_phys(ulong vaddr)
{
    // FIXME
    return cast_vaddr_to_paddr(vaddr);

//     ofw_arg_t trans[4];
//     ofw_arg_t ret = ofw_call("call-method", 4, 5, trans, "translate", ofw_mmu,
//                              (ofw_arg_t)vaddr, 0);
//     panic_if(ret, "OFW unable to translate %p\n", vaddr);
//
//     // If nothing got translated then this address is probably directly mapped
//     if (!trans[0]) {
//         return vaddr;
//     }
//
//     ulong paddr = 0;
//
//     // FIXME: also need to handle endianess
// #if (ARCH_WIDTH == 32)
//     paddr = (ulong)trans[2];
// #elif (ARCH_WIDTH == 64)
//     paddr = (((ulong)trans[2] << 32) | (ulong)trans[3]);
// #else
// #error Unsupported architecture width
// #endif
//
//     return (void *)paddr;
}

// static void *alloc_virt(ulong size, ulong align)
// {
//     ofw_arg_t addr;
//     int ret = (int)ofw_call("call-method", 5, 2, &addr, "claim", ofw_mmu, align, size, (ofw_arg_t)0);
//     panic_if(ret || !(ulong)addr, "OFW unable to allocate virt memory!\n");
//
// //     // FIXME: what about 64-bit?
// // #if (ARCH_WIDTH == 64)
// //     ofw_arg_t addr[2];
// //     ret = ofw_call("call-method", 6, 3, addr, "claim", ofw_mmu, align, size, (ofw_arg_t)0, (ofw_arg_t)0);
// //     ulong addr = (ulong)(((u64)addr[0] << 32) | (u64)addr[1]);
// // #endif
//
//     return (void *)addr;
// }

// static void map_virt_to_phys(void *vaddr, void *paddr, ulong size)
// {
//     ofw_arg_t phys_hi;
//     ofw_arg_t phys_lo;
//
//     // FIXME: also need to handle endianess
//
// #if (ARCH_WIDTH == 32)
//     phys_hi = (ofw_arg_t)paddr;
//     phys_lo = 0;
//
// #elif (ARCH_WIDTH == 64)
//     phys_hi = (ofw_arg_t)((ulong)paddr >> 32);
//     phys_lo = (ofw_arg_t)((ulong)paddr & 0xfffffffful);
//
// #else
//     #error Unsupported architecture width
// #endif
//
//     // FIXME: Do I have align size up to PAGE_SIZE?
//     ofw_arg_t ret = ofw_call("call-method", 7, 1, NULL, "map", ofw_mmu,
//         (ofw_arg_t)-1, size, vaddr, phys_hi, phys_lo);
//
//     panic_if(ret, "OFW unable to map: %p -> %p (%lx)\n", vaddr, paddr, size);
// }

void *obp_alloc_and_map_acc_win(paddr_t paddr, ulong size, ulong align)
{
    // FIXME
    return cast_paddr_to_ptr(paddr);

//     void *vaddr = alloc_virt(size, align);
//     map_virt_to_phys(vaddr, paddr, size);
//
//     return vaddr;
}
