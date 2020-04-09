#include "common/include/arch.h"
#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/devtree.h"
#include "loader/include/firmware.h"


/*
 * OFW data types
 */
#define MEMMAP_MAX_RECORDS              32
#define MAX_OFW_ARGS                    12
#define OFW_TREE_PATH_MAX_LEN           256
#define OFW_TREE_PROPERTY_MAX_NAMELEN   32
#define OFW_TREE_PROPERTY_MAX_VALUELEN  64

typedef unsigned long ofw_arg_t;
typedef unsigned long ofw_prop_t;
typedef unsigned long ofw_ihandle_t;
typedef unsigned long ofw_phandle_t;

struct ofw_args {
    ofw_arg_t service;              // Command name
    ofw_arg_t nargs;                // Number of in arguments
    ofw_arg_t nret;                 // Number of out arguments
    ofw_arg_t args[MAX_OFW_ARGS];   // List of arguments
} packedstruct;

typedef ofw_arg_t (*ofw_entry_t)(struct ofw_args *args);


/*
 * OFW entry point and nodes
 */
static ofw_entry_t client_interface;

static ofw_phandle_t ofw_root;
static ofw_phandle_t ofw_chosen;
static ofw_ihandle_t ofw_mmu;


/*
 * Perform a call to OpenFirmware client interface
 *
 *  service String identifying the service requested.
 *  nargs   Number of input arguments.
 *  nret    Number of output arguments. This includes the return value.
 *  rets    Buffer for output arguments or NULL. The buffer must accommodate nret - 1 items.
 *
 *  return  Return value returned by the client interface.
 *
 */
static ofw_arg_t ofw_call(const char *service, int nargs, int nret,
    ofw_arg_t *rets, ...)
{
    struct ofw_args args;
    args.service = (ofw_arg_t)service;
    args.nargs = nargs;
    args.nret = nret;

    va_list list;
    va_start(list, rets);

    int i;
    for (i = 0; i < nargs; i++) {
        args.args[i] = va_arg(list, ofw_arg_t);
    }

    va_end(list);

    for (i = 0; i < nret; i++) {
        args.args[i + nargs] = 0;
    }

    client_interface(&args);

    for (i = 1; i < nret; i++) {
        rets[i - 1] = args.args[i + nargs];
    }

    return args.args[nargs];
}


/*
 * Helpers
 */
static ofw_phandle_t find_dev(const char *name)
{
    return (ofw_phandle_t)ofw_call("finddevice", 1, 1, NULL, name);
}

static ofw_arg_t pkg_to_path(ofw_phandle_t device, const char *buf, int len)
{
    return ofw_call("package-to-path", 3, 1, NULL, device, buf, len);
}

static ofw_arg_t get_prop(ofw_phandle_t device, const char *name, void *buf, int len)
{
    return ofw_call("getprop", 4, 1, NULL, device, name, buf, len);
}

static ofw_arg_t get_next_prop(ofw_phandle_t device, const char *cur, char *next)
{
    return ofw_call("nextprop", 3, 1, NULL, device, cur, next);
}

static ofw_arg_t get_prop_len(ofw_phandle_t device, const char *name)
{
    return ofw_call("getproplen", 2, 1, NULL, device, name);
}

static ofw_phandle_t get_child_node(ofw_phandle_t node)
{
    return (ofw_phandle_t)ofw_call("child", 1, 1, NULL, node);
}

static ofw_phandle_t get_peer_node(ofw_phandle_t node)
{
    return (ofw_phandle_t)ofw_call("peer", 1, 1, NULL, node);
}


/*
 * Copy device tree
 */
static char path[OFW_TREE_PATH_MAX_LEN + 1];
static char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
static char name_next[OFW_TREE_PROPERTY_MAX_NAMELEN];

static void copy_node(ofw_phandle_t handle, struct devtree_node *parent)
{
    // Process all nodes in the same level
    for (int i = 0; handle && (int)handle != -1;
        handle = get_peer_node(handle), i++
    ) {
        // Root should not have a peer
        if (!parent) {
            panic_if(i > 0, "OFW root node must not have a peer!\n");
        }

        // Get the disambigued name
        int len = (int)pkg_to_path(handle, path, OFW_TREE_PATH_MAX_LEN);
        if (len == -1) {
            continue;
        }
        path[len] = '\0';

        // Find the last slash
        char *node_name = path;
        for (int i = 0; i < len; i++) {
            if (path[i] == '/') {
                node_name = &path[i + 1];
            }
        }

        // Create the devtree node
        //kprintf("Copy node: |%s|\n", node_name);
        struct devtree_node *node = devtree_alloc_node(parent, node_name);

        // Find and copy props
        for (name[0] = '\0'; get_next_prop(handle, name, name_next) == 1 &&
            memcpy(name, name_next, OFW_TREE_PROPERTY_MAX_NAMELEN);
        ) {
            int data_len = (int)get_prop_len(handle, name);
            //kprintf("\tCopy prop: %s, len: %d\n", name, data_len);

            struct devtree_prop *prop = devtree_alloc_prop(node, name, NULL, data_len);
            if (data_len) {
                get_prop(handle, name, devtree_get_prop_data(prop), data_len);
            }
        }

        // Go to child node
        ofw_phandle_t child = get_child_node(handle);
        if (child && (int)child != -1) {
            copy_node(child, node);
        }
    }
}

static void copy_devtree()
{
    copy_node(ofw_root, NULL);
}

static void add_initrd_to_chosen()
{
}


/*
 * Init
 */
static void init_ofw_nodes()
{
    ofw_root = find_dev("/");
    panic_if((int)ofw_root == -1, "OFW unable to find path /");

    ofw_chosen = find_dev("/chosen");
    panic_if((int)ofw_chosen == -1, "OFW unable to find path /chosen");

    int ret = (int)get_prop(ofw_chosen, "mmu", &ofw_mmu, sizeof(ofw_mmu));
    panic_if(ret <= 0, "OFW unable to find MMU");
}

void init_ofw(void *entry)
{
    client_interface = entry;
    init_ofw_nodes();
    copy_devtree();
}

void ofw_add_initrd(void *initrd_start, ulong initrd_size)
{
    // Get chosen node
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        chosen = devtree_alloc_node(devtree_get_root(), "chosen");
    }

    // Initrd
    u64 initrd_start64 = (u64)(ulong)initrd_start;
    u64 initrd_size64 = (u64)initrd_size;
    u64 initrd_end64 = initrd_start64 + initrd_size64;

    if (initrd_start && initrd_size) {
        devtree_alloc_prop_u64(chosen, "initrd-start", initrd_start64);
        devtree_alloc_prop_u64(chosen, "initrd-end", initrd_end64);
    }
}


/*
 * Memory allocation and mapping
 */
paddr_t ofw_translate_virt_to_phys(ulong vaddr)
{
    ofw_arg_t trans[4];
    ofw_arg_t ret = ofw_call("call-method", 4, 5, trans, "translate", ofw_mmu,
                             (ofw_arg_t)vaddr, 0);
    panic_if(ret, "OFW unable to translate %p\n", vaddr);

    // If nothing got translated then this address is probably directly mapped
    if (!trans[0]) {
        return vaddr;
    }

    paddr_t paddr = 0;

    // FIXME: also need to handle endianess
#if (ARCH_WIDTH == 32)
    u32 paddr32 = (u32)trans[2];
    paddr = cast_u32_to_paddr(paddr32);
#elif (ARCH_WIDTH == 64)
    u64 paddr64 = (((u64)trans[2] << 32) | (u64)trans[3]);
    paddr = cast_u64_to_paddr(paddr64);
#else
#error Unsupported architecture width
#endif

    return paddr;
}

static void *alloc_virt(ulong size, ulong align)
{
    ofw_arg_t addr;
    int ret = (int)ofw_call("call-method", 5, 2, &addr, "claim", ofw_mmu, align, size, (ofw_arg_t)0);
    panic_if(ret || !(ulong)addr, "OFW unable to allocate virt memory!\n");

//     // FIXME: what about 64-bit?
// #if (ARCH_WIDTH == 64)
//     ofw_arg_t addr[2];
//     ret = ofw_call("call-method", 6, 3, addr, "claim", ofw_mmu, align, size, (ofw_arg_t)0, (ofw_arg_t)0);
//     ulong addr = (ulong)(((u64)addr[0] << 32) | (u64)addr[1]);
// #endif

    return (void *)addr;
}

static void map_virt_to_phys(void *vaddr, paddr_t paddr, ulong size)
{
    ofw_arg_t phys_hi;
    ofw_arg_t phys_lo;

    // FIXME: also need to handle endianess

#if (ARCH_WIDTH == 32)
    u32 paddr32 = cast_paddr_to_u32(paddr);
    phys_hi = (ofw_arg_t)paddr32;
    phys_lo = 0;

#elif (ARCH_WIDTH == 64)
    u64 paddr64 = cast_paddr_to_u64(paddr);
    phys_hi = (ofw_arg_t)(paddr64 >> 32);
    phys_lo = (ofw_arg_t)(paddr64 & 0xfffffffful);

#else
    #error Unsupported architecture width
#endif

    // FIXME: Do I have align size up to PAGE_SIZE?
    ofw_arg_t ret = ofw_call("call-method", 7, 1, NULL, "map", ofw_mmu,
        (ofw_arg_t)-1, size, vaddr, phys_hi, phys_lo);

    panic_if(ret, "OFW unable to map: %p -> %llx (%lx)\n", vaddr, (u64)paddr, size);
}

void *ofw_alloc_and_map_acc_win(paddr_t paddr, ulong size, ulong align)
{
    void *vaddr = alloc_virt(size, align);
    map_virt_to_phys(vaddr, paddr, size);

    return vaddr;
}
