#include "common/include/inttypes.h"
#include "loader/include/fdt.h"
#include "loader/include/devtree.h"
#include "loader/include/lib.h"


/*
 * This symbol is declared as weak so that the actual FDT can override this
 * The bin2c tool is used to convert the FDT binary to C, and later compiled
 * to the loader ELF
 */
unsigned char __attribute__((weak, aligned(8))) embedded_fdt[] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};


static struct fdt_header *fdt = NULL;


/*
 * Helpers
 */
static inline struct fdt_memrsv_entry *get_memrsv_entry()
{
    return fdt->off_mem_rsvmap ? (void *)fdt + fdt->off_mem_rsvmap : NULL;
}

static inline struct fdt_struct *get_struct_block()
{
    return (void *)fdt + fdt->off_dt_struct;
}

static inline struct fdt_struct *skip_begin_node(struct fdt_struct *node)
{
    struct fdt_struct_begin *begin = (struct fdt_struct_begin *)node;
    ulong addr = (ulong)node + 4 + strlen(begin->name) + 1;
    return (void *)ALIGN_UP(addr, 4ul);
}

static inline struct fdt_struct *skip_prop_node(struct fdt_struct *node)
{
    struct fdt_struct_prop *prop = (struct fdt_struct_prop *)node;
    ulong addr = (ulong)node + sizeof(struct fdt_struct_prop) + prop->len;
    return (void *)ALIGN_UP(addr, 4ul);
}

static inline void *get_prop_value(struct fdt_struct_prop *prop)
{
    return (void *)prop + sizeof(struct fdt_struct_prop);
}

static inline char *get_str(u32 off)
{
    return (void *)fdt + fdt->off_dt_strings + off;
}


/*
 * Init
 */
static inline struct devtree_node *create_devtree_node(
    struct devtree_node *parent, struct fdt_struct *fdt_node)
{
    struct fdt_struct_begin *begin = (struct fdt_struct_begin *)fdt_node;
    return devtree_alloc_node(parent, begin->name);
}

static inline void create_devtree_prop(
    struct devtree_node *node, struct fdt_struct *fdt_node)
{
    struct fdt_struct_prop *prop = (struct fdt_struct_prop *)fdt_node;
    devtree_alloc_prop(node, get_str(prop->off_name),
        get_prop_value(prop), prop->len);
}

static void fix_endian()
{
    // Fix header
    fdt->total_size = swap_big_endian32(fdt->total_size);
    fdt->off_dt_struct = swap_big_endian32(fdt->off_dt_struct);
    fdt->off_dt_strings = swap_big_endian32(fdt->off_dt_strings);
    fdt->off_mem_rsvmap = swap_big_endian32(fdt->off_mem_rsvmap);
    fdt->version = swap_big_endian32(fdt->version);
    fdt->last_comp_version = swap_big_endian32(fdt->last_comp_version);
    fdt->boot_cpuid_phys = swap_big_endian32(fdt->boot_cpuid_phys);
    fdt->size_dt_strings = swap_big_endian32(fdt->size_dt_strings);
    fdt->size_dt_struct = swap_big_endian32(fdt->size_dt_struct);

//     // Fix memrsv block
//     for (struct fdt_memrsv_entry *memrsv = get_memrsv_entry();
//         memrsv && (memrsv->addr || memrsv->size); memrsv++
//     ) {
//         memrsv->addr = swap_big_endian64(memrsv->addr);
//         memrsv->size = swap_big_endian64(memrsv->size);
//     }

    // Fix struct block
    struct fdt_struct *cur_fdt_node = get_struct_block();
    while (cur_fdt_node) {
        cur_fdt_node->token = swap_big_endian32(cur_fdt_node->token);

        switch (cur_fdt_node->token) {
        case FDT_BEGIN_NODE:
            cur_fdt_node = skip_begin_node(cur_fdt_node);
            break;
        case FDT_END_NODE:
            cur_fdt_node++;
            break;
        case FDT_PROP: {
            struct fdt_struct_prop *prop = (void *)cur_fdt_node;
            prop->len = swap_big_endian32(prop->len);
            prop->off_name = swap_big_endian32(prop->off_name);
            cur_fdt_node = skip_prop_node(cur_fdt_node);
            break;
        }
        case FDT_NOP:
            cur_fdt_node++;
            break;
        case FDT_END:
            cur_fdt_node = NULL;
            break;
        default:
            panic("Bad FDT node, token: %d\n", cur_fdt_node->token);
            break;
        }
    }
}

static void create_devtree()
{
    struct fdt_struct *cur_fdt_node = get_struct_block();
    struct devtree_node *cur_devtree_node = NULL;

    while (cur_fdt_node) {
        switch (cur_fdt_node->token) {
        case FDT_BEGIN_NODE:
            cur_devtree_node =
                create_devtree_node(cur_devtree_node, cur_fdt_node);
            cur_fdt_node = skip_begin_node(cur_fdt_node);
            break;
        case FDT_END_NODE:
            cur_devtree_node = devtree_get_parent_node(cur_devtree_node);
            cur_fdt_node++;
            break;
        case FDT_PROP:
            create_devtree_prop(cur_devtree_node, cur_fdt_node);
            cur_fdt_node = skip_prop_node(cur_fdt_node);
            break;
        case FDT_NOP:
            cur_fdt_node++;
            break;
        case FDT_END:
            cur_fdt_node = NULL;
            break;
        default:
            panic("Bad FDT node, token: %d\n", cur_fdt_node->token);
            break;
        }
    }
}

static void next_name(char *name, int digits)
{
    name[digits - 1]++;
    for (int d = digits - 1; name[d] > '9' && d >= 1; d--) {
        name[d] = '0';
        name[d - 1]++;
    }
}

static void create_memrsv_node()
{
    struct fdt_memrsv_entry *entry = get_memrsv_entry();
    if (!entry || (!entry->addr && !entry->size)) {
        return;
    }

    struct devtree_node *root = devtree_get_root();
    struct devtree_node *memsrv = devtree_alloc_node(root, "memrsv-block");
    char name[4] = { '0', '0', '1', 0 };

    for (; entry && (entry->addr || entry->size);
         entry++, next_name(name, 3)
    ) {
        u64 data[2] = { entry->addr, entry->size };
        devtree_alloc_prop(memsrv, name, data, sizeof(data));
    }
}

static void init_fdt()
{
    fix_endian();
    create_devtree();
    create_memrsv_node();
}


/*
 * General
 */
int is_fdt_header(void *fdt)
{
    struct fdt_header *hdr = (struct fdt_header *)fdt;
    return hdr && swap_big_endian32(hdr->magic) == FDT_HEADER_MAGIC;
}

int copy_fdt(void *buf, void *src, size_t size)
{
    if (!is_fdt_header(src)) {
        return -1;
    }

    u32 fdt_size = swap_big_endian32(((struct fdt_header *)src)->total_size);
    if (fdt_size > size) {
        return -1;
    }

    memcpy(buf, src, fdt_size);

    return fdt_size;
}

void init_supplied_fdt(void *supplied_fdt)
{
    struct fdt_header *hdr = supplied_fdt;
    panic_not(is_fdt_header(hdr), "Supplied FDT is invalid!\n");

    fdt = hdr;
    init_fdt();
}

static struct fdt_header *search_appended_fdt()
{
    if (is_fdt_header(embedded_fdt)) {
        return (struct fdt_header *)embedded_fdt;
    }

    return NULL;
}

void init_appended_fdt()
{
    if (devtree_get_root()) {
        return;
    }

    struct fdt_header *hdr = search_appended_fdt();
    panic_not(is_fdt_header(hdr), "Unable to find appended FDT!\n");

    fdt = hdr;
    init_fdt();
}
