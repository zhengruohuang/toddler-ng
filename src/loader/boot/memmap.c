#include "common/include/inttypes.h"
#include "common/include/memmap.h"
#include "loader/include/devtree.h"
#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/kprintf.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/mem.h"


#define MAX_MEMMAP_SIZE 64


static struct loader_memmap_entry memmap_entries[MAX_MEMMAP_SIZE];
static struct loader_memmap memmap = {
    .num_slots = MAX_MEMMAP_SIZE,
    .num_entries = 0,
    .entries = memmap_entries
};

static u64 memstart = 0, memsize = 0;


/*
 * Phys mem range
 */
u64 get_memmap_range(u64 *start)
{
    if (start) {
        *start = memstart;
    }
    return memsize;
}


/*
 * Allocation
 */
paddr_t memmap_alloc_phys(ulong size, ulong align)
{
    u64 paddr_u64 = find_free_memmap_region_for_palloc(size, align);
    paddr_t paddr = cast_u64_to_paddr(paddr_u64);
    return paddr;
}


/*
 * Construct memmap
 */
static void claim_coreimg()
{
    int coreimg_size = 0;
    ulong vaddr = (ulong)find_supplied_coreimg(&coreimg_size);

    if (coreimg_size && vaddr) {
        paddr_t paddr = firmware_translate_virt_to_phys(vaddr);
        claim_memmap_region((u64)paddr, (u64)coreimg_size, MEMMAP_USED);
    }
}

extern int __start;
extern int __end;

static void claim_loader()
{
    //struct loader_arch_funcs *funcs = get_loader_arch_funcs();

    //ulong vaddr = (ulong)&__start;
    //if (vaddr >= funcs->reserved_stack_size) {
    //    vaddr -= funcs->reserved_stack_size;
    //} else {
    //    vaddr = 0;
    //}

    //vaddr = ALIGN_DOWN(vaddr, funcs->page_size);

    //ulong vaddr_end = (ulong)&__end;
    //vaddr_end = ALIGN_UP(vaddr_end, funcs->page_size);

    //ulong size = (ulong)&__end - vaddr;
    //ulong size = vaddr_end - vaddr;

    ulong vaddr_start = align_down_vaddr((ulong)&__start, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr((ulong)&__end, PAGE_SIZE);
    ulong size = vaddr_end - vaddr_start;

    paddr_t paddr = firmware_translate_virt_to_phys(vaddr_start);
    claim_memmap_region((u64)paddr, (u64)size, MEMMAP_USED);
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

        kprintf("Memory reserve, addr: %llx, size: %llx\n", addr, size);
        claim_memmap_region(addr, size, MEMMAP_USED);
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
            claim_memmap_region(prev_end, addr - prev_end, MEMMAP_USED);
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
    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
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

        kprintf("Memory region @ %llx, size: %llx, idx: %d\n", addr, size, reg_idx);

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

        // Claim the region
        claim_memmap_region(addr, size, MEMMAP_USABLE);
        tag_memmap_region(addr, size, MEMMAP_TAG_NORMAL);

        size_up_to_now += size;
    } while (reg_idx > 0 && (!memsize || size_up_to_now < memsize));

    //panic_if(!entry_count, "Unable to create memory map!");

    if (!memsize) {
        memsize = size_up_to_now;
    }

    kprintf("Total memory size: %llx\n", memsize);
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
    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
    if (funcs->phys_mem_range_min > memstart) {
        memstart = funcs->phys_mem_range_min;
    }
    if (funcs->phys_mem_range_max && funcs->phys_mem_range_max > memstart &&
        funcs->phys_mem_range_max - memstart < memsize
    ) {
        memsize = funcs->phys_mem_range_max;
    }

    claim_memmap_region(memstart, memsize, MEMMAP_USABLE);
    tag_memmap_region(memstart, memsize, MEMMAP_TAG_NORMAL);
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

void init_memmap()
{
    memzero(memmap_entries, sizeof(memmap_entries));
    init_libk_memmap(&memmap);

    // Create all memory map
    create_all();
    print_memmap();

    // Claim already used regions
    //__kprintf("firmware\n");
    claim_firmware();
    //print_memmap();

    //__kprintf("memsrv\n");
    claim_memrsv();
    //print_memmap();

    //__kprintf("loader\n");
    claim_loader();
    //print_memmap();

    //__kprintf("coreimg\n");
    claim_coreimg();

    // Done
    print_memmap();
}
