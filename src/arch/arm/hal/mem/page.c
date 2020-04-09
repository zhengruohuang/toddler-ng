#include "common/include/inttypes.h"
#include "common/include/page.h"
#include "hal/include/lib.h"
#include "hal/include/kernel.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"


/*
 * Get physical address of a user virtual address
 */
paddr_t translate(void *page_table, ulong vaddr)
{
    // L1 table
    struct l1table *l1tab = page_table;
    int index = GET_L1PTE_INDEX(vaddr);
    struct l1page_table_entry *l1entry = &l1tab->entries[index];

    // Not mapped yet
    if (!l1entry->value) {
        return 0;
    }

    // 1MB section
    if (l1entry->block.present) {
        ppfn_t bfn = l1entry->block.bfn;
        paddr_t paddr = pbfn_to_paddr(bfn);
        paddr |= (paddr_t)get_vblock_offset(vaddr);
        return paddr;
    }

    // L2 table
    assert(l1entry->pte.present);
    ppfn_t pfn = l1entry->pte.pfn;
    paddr_t paddr = ppfn_to_paddr(pfn);
    struct l2table *l2tab = cast_paddr_to_ptr(paddr);

    index = GET_L2PTE_INDEX(vaddr);
    struct l2page_table_entry *l2entry = &l2tab->entries[index];

    // Not mapped yet
    if (!l2entry->value) {
        return 0;
    }

    // Paddr
    pfn = l2entry->pfn;
    paddr = ppfn_to_paddr(pfn);
    paddr |= (paddr_t)get_vpage_offset(vaddr);

    return paddr;
}


/*
 * Map
 */
static struct l2table *alloc_l2table(palloc_t palloc)
{
    ppfn_t ppfn = palloc(1);
    paddr_t paddr = ppfn_to_paddr(ppfn);
    void *ptr = cast_paddr_to_ptr(paddr);
    return ptr;
}

static void map_page(void *page_table, ulong vaddr, paddr_t paddr, int block,
    int cache, int exec, int write, int kernel, int override, palloc_t palloc)
{
    struct l1table *l1table = page_table;
    int l1idx = GET_L1PTE_INDEX((ulong)vaddr);
    struct l1page_table_entry *l1entry = &l1table->entries[l1idx];

    // 1-level block mapping
    if (block) {
        if (l1entry->block.present && !override) {
            panic_if((ulong)l1entry->block.bfn != (u32)paddr_to_pbfn(paddr) ||
                l1entry->block.no_exec != !exec ||
                l1entry->block.read_only != !write ||
                l1entry->block.cacheable != cache,
                "Trying to change an existing mapping from BFN %x to %x!",
                l1entry->block.bfn, (u32)paddr_to_pbfn(paddr));
        }

        else {
            l1entry->block.present = 1;
            l1entry->block.bfn = (u32)paddr_to_pbfn(paddr);
            l1entry->block.no_exec = !exec;
            l1entry->block.read_only = !write;
            l1entry->block.user_access = 0;   // Kernel page
            l1entry->block.user_write = 1;    // Kernel page write is determined by read_only
            l1entry->block.cacheable = cache;
            l1entry->block.cache_inner = l1entry->block.cache_outer = 0x1;
                                // write-back, write-allocate when cacheable
                                // See TEX[2], TEX[1:0] and CB
        }
    }

    // 2-level page mapping
    else {
        struct l2table *l2table = NULL;
        if (!l1entry->pte.present) {
            l2table = alloc_l2table(palloc);
            memzero(l2table, L2PAGE_TABLE_SIZE);

            paddr_t l2_paddr = cast_ptr_to_paddr(l2table);
            l1entry->pte.present = 1;
            l1entry->pte.pfn = paddr_to_ppfn(l2_paddr);
        } else {
            paddr_t l2_paddr = ppfn_to_paddr(l1entry->pte.pfn);
            l2table = cast_paddr_to_ptr(l2_paddr);
        }

        int l2idx = GET_L2PTE_INDEX(vaddr);
        struct l2page_table_entry *l2entry = &l2table->entries[l2idx];

        if (l2entry->present && !override) {
            panic_if((ulong)l2entry->pfn != (u32)paddr_to_ppfn(paddr),
                "Trying to change an existing mapping from PFN %x to %x!",
                l2entry->pfn, (u32)paddr_to_ppfn(paddr));

            panic_if((int)l2entry->cacheable != cache,
                "Trying to change cacheable attribute!");

            if (exec) l2entry->no_exec = 0;
            if (write) l2entry->read_only = 0;
        }

        else {
            l2entry->present = 1;
            l2entry->pfn = (u32)paddr_to_ppfn(paddr);
            l2entry->no_exec = !exec;
            l2entry->read_only = !write;
            l2entry->user_access = 0;   // Kernel page
            l2entry->user_write = 1;    // Kernel page write is determined by read_only
            l2entry->cacheable = cache;
            l2entry->cache_inner = l2entry->cache_outer = 0x1;
                                // write-back, write-allocate when cacheable
                                // See TEX[2], TEX[1:0] and CB
        }

        for (int i = 0; i < 3; i++) {
            l2table->entries_dup[i][l2idx].value = l2entry->value;
        }
    }
}

int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
              int cache, int exec, int write, int kernel, int override,
              palloc_t palloc)
{
    //kprintf("Map, page_dir_pfn: %lx, vaddr @ %lx, paddr @ %lx, size: %ld, "
    //        "cache: %d, exec: %d, write: %d, kernel: %d, override: %d\n",
    //        page_dir_pfn, vaddr, paddr, size, cache, exec, write, kernel, override
    //);

    ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
    paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);

    int mapped_pages = 0;

    paddr_t cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
        // 1MB block
        if (ALIGNED(cur_vaddr, L1BLOCK_SIZE) &&
            ALIGNED(cur_paddr, L1BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L1BLOCK_SIZE
        ) {
            map_page(page_table, cur_vaddr, cur_paddr, 1,
                cache, exec, write, kernel, override, palloc);

            mapped_pages += L1BLOCK_PAGE_COUNT;
            cur_vaddr += L1BLOCK_SIZE;
            cur_paddr += L1BLOCK_SIZE;
        }

        // 4KB page
        else {
            map_page(page_table, cur_vaddr, cur_paddr, 0,
                cache, exec, write, kernel, override, palloc);

            mapped_pages++;
            cur_vaddr += PAGE_SIZE;
            cur_paddr += PAGE_SIZE;
        }
    }

    return mapped_pages;
}


/*
 * Unmap
 */
static void unmap_page(void *page_table, ulong vaddr, paddr_t paddr, int block)
{
    // L1 table
    struct l1table *l1tab = page_table;
    int l1index = GET_L1PTE_INDEX(vaddr);
    struct l1page_table_entry *l1entry = &l1tab->entries[l1index];
    assert(l1entry->value);

    if (block) {
        assert(l1entry->block.present);

        // Unmap L1 entry
        assert(l1entry->block.bfn == (u32)paddr_to_pbfn(paddr));
        l1entry->value = 0;
    } else {
        assert(l1entry->pte.present);

        ppfn_t pfn = l1entry->pte.pfn;
        paddr_t l2_paddr = ppfn_to_paddr(pfn);
        struct l2table *l2tab = cast_paddr_to_ptr(l2_paddr);

        int l2index = GET_L2PTE_INDEX(vaddr);
        struct l2page_table_entry *l2entry = &l2tab->entries[l2index];
        assert(l2entry->present);

        // Unmap L2 entry
        assert(l2entry->pfn == (u32)paddr_to_pbfn(paddr));
        l2entry->value = 0;

        // Free L2 page if needed
        int free_l2 = 1;
        for (int i = 0; i < L2PAGE_TABLE_ENTRY_COUNT; i++) {
            if (l2tab->value_u32[i]) {
                free_l2 = 0;
                break;
            }
        }

        if (free_l2) {
            paddr_t l2_paddr = cast_ptr_to_paddr(l2tab);
            int num_free_pages = kernel_pfree(paddr_to_ppfn(l2_paddr));
            panic_if(num_free_pages != L2PAGE_TABLE_SIZE_IN_PAGES,
                "Inconsistent num of freed pages");

            l1entry->value = 0;
        }
    }

    // Free L1 page if needed
    int free_l1 = 1;
    for (int i = 0; i < L1PAGE_TABLE_ENTRY_COUNT; i++) {
        if (l1tab->value_u32[i]) {
            free_l1 = 0;
            break;
        }
    }

    if (free_l1) {
        paddr_t l1_paddr = cast_ptr_to_paddr(l1tab);
        int num_free_pages = kernel_pfree(paddr_to_pbfn(l1_paddr));
        panic_if(num_free_pages != L1PAGE_TABLE_SIZE_IN_PAGES,
            "Inconsistent num of freed pages");
    }
}

int unumap_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size)
{
    //kprintf("To unmap, pfn: %u, vaddr: %u, paddr: %u, size: %u\n",
    //        page_dir_pfn, vaddr, paddr, length);

    ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
    paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);

    int unmapped_pages = 0;

    paddr_t cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end;) {
        // 1MB block
        if (is_vaddr_aligned(cur_vaddr, L1BLOCK_SIZE) &&
            is_paddr_aligned(cur_paddr, L1BLOCK_SIZE) &&
            vaddr_end - cur_vaddr >= L1BLOCK_SIZE
        ) {
            unmap_page(page_table, cur_vaddr, cur_paddr, 1);

            unmapped_pages += L1BLOCK_PAGE_COUNT;
            cur_vaddr += L1BLOCK_SIZE;
            cur_paddr += L1BLOCK_SIZE;
        }

        // 4KB page
        else {
            unmap_page(page_table, cur_vaddr, cur_paddr, 0);

            unmapped_pages++;
            cur_vaddr += PAGE_SIZE;
            cur_paddr += PAGE_SIZE;
        }
    }

    return unmapped_pages;
}
