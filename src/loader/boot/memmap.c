#include "common/include/inttypes.h"
#include "loader/include/devtree.h"
#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/lprintf.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"


#define MAX_MEMMAP_SIZE 64


static int entry_count = 0;
static struct memmap_entry memmap[MAX_MEMMAP_SIZE];
static u64 memstart = 0, memsize = 0;


static struct memmap_entry *insert_entry(int idx)
{
    if (idx > entry_count) {
        return NULL;
    }

    for (int i = entry_count - 1; i >= idx; i--) {
        memmap[i + 1] = memmap[i];
    }

    entry_count++;
    panic_if(entry_count >= MAX_MEMMAP_SIZE, "Memmap overflow\n");

    return &memmap[idx];
}

static void remove_entry(int idx)
{
    if (idx > entry_count) {
        return;
    }

    int i = 0;
    for (i = idx; i < entry_count - 1; i++) {
        memmap[i] = memmap[i + 1];
    }
    memzero(&memmap[i], sizeof(struct memmap_entry));

    entry_count--;
}

static void merge_consecutive_regions()
{
    for (int i = 0; i < entry_count - 1; ) {
        struct memmap_entry *entry = &memmap[i];
        struct memmap_entry *next = &memmap[i + 1];

        if (entry->start + entry->size == next->start &&
            entry->flags == next->flags
        ) {
            entry->size += next->size;
            remove_entry(i + 1);
        } else {
            i++;
        }
    }
}

static void claim_region(u64 start, u64 size, int flag)
{
    u64 end = start + size;

    for (int i = 0; i < entry_count && size; i++) {
        struct memmap_entry *entry = &memmap[i];
        u64 entry_end = entry->start + entry->size;

        if (end <= entry->start) {
            size = 0;
        }

        else if (start <= entry->start && end >= entry_end) {
            entry->flags = flag;

            size -= entry_end - start;
            start = entry_end;
        } else if (start <= entry->start &&
            end > entry->start && end < entry_end
        ) {
            struct memmap_entry *claimed = insert_entry(i);
            claimed->start = entry->start;
            claimed->size = end - entry->start;
            claimed->flags = flag;

            entry = &memmap[++i];
            entry->start = end;
            entry->size -= claimed->size;

            size = 0;
        }

        else if (start > entry->start && start < entry_end &&
            end >= entry_end
        ) {
            struct memmap_entry *claimed = insert_entry(++i);
            claimed->start = start;
            claimed->size = entry_end - start;
            claimed->flags = flag;

            entry->size = start - entry->start;

            size -= entry_end - start;
            start = entry_end;
        } else if (start > entry->start && end < entry_end) {
            struct memmap_entry *claimed = insert_entry(++i);
            claimed->start = start;
            claimed->size = size;
            claimed->flags = flag;

            entry->size = start - entry->start;

            struct memmap_entry *next = insert_entry(++i);
            next->start = end;
            next->size = entry_end - end;
            next->flags = entry->flags;

            size = 0;
        }
    }

    merge_consecutive_regions();
}

static void claim_coreimg()
{
    int coreimg_size = 0;
    ulong vaddr = (ulong)find_supplied_coreimg(&coreimg_size);

    if (coreimg_size && vaddr) {
        ulong paddr = (ulong)firmware_translate_virt_to_phys((void *)vaddr);
        claim_region((u64)paddr, (u64)coreimg_size, MEMMAP_LOADER);
    }
}

extern int __start;
extern int __end;

static void claim_loader()
{
    struct loader_arch_funcs *funcs = get_arch_funcs();

    ulong vaddr = (ulong)&__start;
    if (vaddr >= funcs->reserved_stack_size) {
        vaddr -= funcs->reserved_stack_size;
    } else {
        vaddr = 0;
    }

    ulong size = (ulong)&__end - vaddr;

    ulong paddr = (ulong)firmware_translate_virt_to_phys((void *)vaddr);
    claim_region((u64)paddr, (u64)size, MEMMAP_LOADER);
}

static void claim_memrsv()
{
    struct devtree_node *memrsv = devtree_walk("/memrsv-block");
    if (!memrsv) {
        return;
    }

    for (struct devtree_prop *memrsv_entry = devtree_get_prop(memrsv);
        memrsv_entry; memrsv_entry = devtree_get_next_prop(memrsv_entry)
    ) {
       u64 *data = devtree_get_prop_data(memrsv_entry);
       u64 addr = swap_big_endian64(*data++);
       u64 size = swap_big_endian64(*data);
       claim_region(addr, size, MEMMAP_USED);
    }
}

static void claim_firmware()
{
    struct devtree_node *memory = devtree_walk("/memory");
    if (!devtree_find_prop(memory, "available")) {
        return;
    }

    int avail_idx = 0;
    u64 addr = 0, size = 0, prev_end = 0;

    do {
        avail_idx = devtree_get_translated_addr(memory, avail_idx, "available",
            &addr, &size);
        panic_if(avail_idx == -1, "Unable to get avail in memory node");

        if (addr) {
            claim_region(prev_end, addr, MEMMAP_USED);
        }

        prev_end = addr + size;
    } while (avail_idx > 0);
}

static int is_valid_memory_node(struct devtree_node *memory)
{
    int reg_idx = 0;
    u64 addr = 0, size = 0;

    do {
        reg_idx = devtree_get_translated_reg(memory, reg_idx, &addr, &size);
        if (size) {
            return 1;
        }
    } while (reg_idx > 0);

    return 0;
}

static void create_all_from_memory_node(struct devtree_node *chosen,
    struct devtree_node *memory)
{
    // Firmware supplied memory start and memory size
    struct devtree_prop *fw_memstart = devtree_find_prop(chosen, "memstart");
    if (fw_memstart) {
        memstart = devtree_get_prop_data_u64(fw_memstart);
    }

    struct devtree_prop *fw_memsize = devtree_find_prop(chosen, "memsize");
    if (fw_memsize) {
        memsize = devtree_get_prop_data_u64(fw_memsize);
    }

    // Adjust memstart and memsize
    struct loader_arch_funcs *funcs = get_arch_funcs();
    if (funcs->phys_mem_range_min > memstart) {
        memstart = funcs->phys_mem_range_min;
    }
    if (funcs->phys_mem_range_max && funcs->phys_mem_range_max > memstart &&
        funcs->phys_mem_range_max - memstart < memsize
    ) {
        memsize = funcs->phys_mem_range_max;
    }

    // Go through memory nodes
    int memstart_found = 0;
    int reg_idx = 0;
    u64 addr = 0, size = 0;
    u64 size_up_to_now = 0;

    do {
        reg_idx = devtree_get_translated_reg(memory, reg_idx, &addr, &size);
        if (!size) {
            continue;
        }

        lprintf("Memory region @ %llx, size: %llx, idx: %d\n", addr, size, reg_idx);

        // Adjust start addr and memstart
        if (addr + size <= memstart) {
            continue;
        } else if (addr < memstart) {
            size -= memstart - addr;
            addr = memstart;
        } else if (!memstart_found) {
            memstart = addr;
            memstart_found = 1;
        }

        // Adjust size
        if (memsize && size_up_to_now + size > memsize) {
            size = memsize - size_up_to_now;
        }

        // See if we can extend the previous entry
        int extend = 0;
        struct memmap_entry *entry = &memmap[entry_count];
        if (entry_count) {
            entry = &memmap[entry_count - 1];
            u64 prev_end = entry->start + entry->size;
            if (prev_end >= addr && addr + size >= prev_end) {
                extend = 1;
                size = addr + size - prev_end;
                addr = prev_end;
            }
        }

        // Update/Create the entry
        if (extend) {
            entry->size += size;
        } else {
            entry->start = addr;
            entry->size = size;
            entry->flags = MEMMAP_USABLE;

            entry_count++;
        }

        size_up_to_now += size;
    } while (reg_idx > 0 && (!memsize || size_up_to_now < memsize));

    panic_if(!entry_count, "Unable to create memory map!");

    if (!memsize) {
        memsize = size_up_to_now;
    }
}

static void create_all_from_chosen_node(struct devtree_node *chosen)
{
    struct devtree_prop *fw_memstart = devtree_find_prop(chosen, "memstart");
    struct devtree_prop *fw_memsize = devtree_find_prop(chosen, "memsize");
    panic_if(!fw_memstart || !fw_memsize,
        "Unable to find memstart or memsize prop!\n");

    memstart = devtree_get_prop_data_u64(fw_memstart);
    memsize = devtree_get_prop_data_u64(fw_memsize);

    // Adjust memstart and memsize
    struct loader_arch_funcs *funcs = get_arch_funcs();
    if (funcs->phys_mem_range_min > memstart) {
        memstart = funcs->phys_mem_range_min;
    }
    if (funcs->phys_mem_range_max && funcs->phys_mem_range_max > memstart &&
        funcs->phys_mem_range_max - memstart < memsize
    ) {
        memsize = funcs->phys_mem_range_max;
    }

    struct memmap_entry *entry = &memmap[0];
    entry->start = devtree_get_prop_data_u64(fw_memstart);
    entry->size = memsize;
    entry->flags = MEMMAP_USABLE;
    entry_count++;
}

static void create_all()
{
    struct devtree_node *chosen = devtree_walk("/chosen");
    panic_if(!chosen, "Unable to find /chosen node!\n");

    struct devtree_node *memory = devtree_walk("/memory");
    if (is_valid_memory_node(memory)) {
        create_all_from_memory_node(chosen, memory);
    } else {
        create_all_from_chosen_node(chosen);
    }
}

static void print_memmap()
{
    for (int i = 0; i < entry_count; i++) {
        struct memmap_entry *entry = &memmap[i];
        lprintf("Memory region #%d @ %llx - %llx, size: %llx, flag: %d\n", i,
            entry->start, entry->start + entry->size, entry->size, entry->flags
        );
    }
}

void init_memmap()
{
    memzero(memmap, sizeof(memmap));

    // Create all memory map
    create_all();

    // Claim already used regions
    claim_firmware();
    claim_memrsv();
    claim_loader();
    claim_coreimg();

    // Merge
    merge_consecutive_regions();

    // Done
    print_memmap();
}

const struct memmap_entry *get_memmap(int *num_entries)
{
    if (num_entries) {
        *num_entries = entry_count;
    }

    return memmap;
}

u64 get_memmap_range(u64 *start)
{
    if (start) {
        *start = memstart;
    }
    return memsize;
}

/*
 * Allocates and returns physical address
 *  ``align'' must be a power of 2
 */
void *memmap_alloc_phys(ulong size, ulong align)
{
    if (!size) {
        return NULL;
    }

    if (align) {
        panic_if(popcount(align) != 1, "Must align to power of 2!");
        size = ALIGN_UP(size, align);
    }

    for (int i = 0; i < entry_count; i++) {
        struct memmap_entry *entry = &memmap[i];
        if (entry->flags == MEMMAP_USABLE && entry->size >= size) {
            ulong aligned_start = (ulong)entry->start;
            if (align) {
                aligned_start = ALIGN_UP(entry->start, align);
            }

            u64 entry_end = entry->start + entry->size;
            if (aligned_start < entry_end &&
                entry_end - aligned_start >= size
            ) {
                claim_region(aligned_start, size, MEMMAP_USED);

                return (void *)aligned_start;
            }
        }
    }

    return NULL;
}
