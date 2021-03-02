#include "common/include/abi.h"
#include "loader/include/loader.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"


/*
 * Generic page talbe
 */
#if 0

#include "common/include/atomic.h"
#include "common/include/page.h"

static inline int page_get_block_level(ulong vaddr, paddr_t paddr, ulong size,
                                       ulong *block_page_count)
{
    for (int level = 1; level <= PAGE_LEVELS; level++) {
        ulong page_count = page_get_block_page_count(level);
        if (page_count) {
            ulong block_size = page_count << PAGE_BITS;

            if (is_vaddr_aligned(vaddr, block_size) &&
                is_paddr_aligned(paddr, block_size) &&
                size >= block_size
            ) {
                if (block_page_count) *block_page_count = page_count;
                return level;
            }
        }
    }

    if (block_page_count) *block_page_count = 1;
    return PAGE_LEVELS;
}


/*
 * Physical PFN to direct-accessible pointer
 */
static inline void *page_ppfn_to_ptr(ppfn_t ppfn)
{
    paddr_t paddr = ppfn_to_paddr(ppfn);
    void *ptr = NULL;

    struct loader_arch_funcs *funcs = get_loader_arch_funcs();
    if (funcs->has_direct_access) {
        ptr = (void *)funcs->phys_to_access_win(paddr);
    } else {
        ptr = cast_paddr_to_ptr(paddr);
    }

    return ptr;
}


/*
 * Get physical address a PTE points to
 */
static inline ulong page_get_block_vmask(int level)
{
    ulong vmask = PAGE_SIZE - 0x1;
    vmask |= (page_get_block_page_count(level) - 0x1ul) << PAGE_BITS;
    return vmask;
}

static inline paddr_t page_get_pte_dest_paddr(void *entry, int level, ulong vaddr)
{
    ppfn_t ppfn = page_get_pte_dest_ppfn(entry, level);
    ulong vmask = page_get_block_vmask(level);

    paddr_t paddr = ppfn_to_paddr(ppfn);
    paddr |= cast_vaddr_to_paddr(vaddr & vmask);

    return paddr;
}


/*
 * Alloc
 */
static inline void *page_alloc_table(int level, ppfn_t *ppfn)
{
    int num_pages = page_get_num_table_pages(level);
    paddr_t paddr = memmap_alloc_phys_page(num_pages);
    ppfn_t pfn = paddr_to_ppfn(paddr);
    if (ppfn) {
        *ppfn = pfn;
    }

    void *ptr = page_ppfn_to_ptr(pfn);
    memzero(ptr, num_pages * PAGE_SIZE);

    return ptr;
}


/*
 * Translate
 */
static paddr_t _translate_attri(void *page_table, ulong vaddr, int need_attri,
                                int *exec, int *read, int *write, int *cache, int *kernel)
{
//     kprintf("Translate, page_table: %lx, vaddr @ %lx\n", page_table, vaddr);

    atomic_mb();

    int level = 0;
    void *pte = NULL;
    paddr_t paddr = 0; // TODO: fix HAL as well

    for (level = 1; level < PAGE_LEVELS; level++) {
        pte = page_get_pte(page_table, level, vaddr);

        // Check if mapping is valid
        if (!page_is_pte_valid(pte, level)) {
            return 0;
        }

        // Move to next level
        if (!page_is_pte_leaf(pte, level)) {
            ppfn_t next_ppfn = page_get_pte_next_table(pte, level);
            page_table = page_ppfn_to_ptr(next_ppfn);
        }

        // Done at this level
        else {
            paddr = page_get_pte_dest_paddr(pte, level, vaddr);
            if (need_attri) {
                page_get_pte_attri(pte, level, vaddr, exec, read, write, cache, kernel);
            }
//             kprintf("  Translated1, page_table: %lx, vaddr @ %lx, paddr @ %llx\n",
//                     page_table, vaddr, (u64)paddr);
            return paddr;
        }
    }

    // The final level
    pte = page_get_pte(page_table, level, vaddr);
    if (!page_is_pte_valid(pte, level)) {
        return 0;
    }

    assert(page_is_pte_leaf(pte, level));

    paddr = page_get_pte_dest_paddr(pte, level, vaddr);
    if (need_attri) {
        page_get_pte_attri(pte, level, vaddr, exec, read, write, cache, kernel);
    }

//     kprintf("  Translated2, page_table: %lx, vaddr @ %lx, paddr @ %llx\n",
//             page_table, vaddr, (u64)paddr);
    return paddr;
}

paddr_t translate_attri(void *page_table, ulong vaddr,
                        int *exec, int *read, int *write, int *cache)
{
    return _translate_attri(page_table, vaddr, 1, exec, read, write, cache, NULL);
}

paddr_t translate(void *page_table, ulong vaddr)
{
    return _translate_attri(page_table, vaddr, 0, NULL, NULL, NULL, NULL, NULL);
}


/*
 * Map
 */
static void map_page(void *page_table, ulong vaddr, paddr_t paddr, int block,
                     int cache, int exec, int write, int kernel, int override)
{
//     kprintf("  Map page, page_table: %lx, vaddr @ %lx, paddr @ %llx, block: %d"
//             ", cache: %d, exec: %d, write: %d, kernel: %d, override: %d\n",
//             page_table, vaddr, (u64)paddr, block,
//             cache, exec, write, 1, 0);

    ppfn_t ppfn = paddr_to_ppfn(paddr);
    int level = 0;
    void *pte = NULL;

    for (level = 1; level < block; level++) {
        pte = page_get_pte(page_table, level, vaddr);

        if (!page_is_pte_valid(pte, level)) {
            ppfn_t next_ppfn = 0;
            page_table = page_alloc_table(level + 1, &next_ppfn);
            assert(next_ppfn);
            assert(page_table);
            page_set_pte_next_table(pte, level, next_ppfn);
        } else {
            if (page_is_pte_leaf(pte, level)) {
                int err = page_compare_pte_attri(pte, level, ppfn,
                                                 exec, 1, write, cache, 1);
                panic_if(err, "Incompatible mapping!");
            }

            ppfn_t next_ppfn = page_get_pte_next_table(pte, level);
            page_table = page_ppfn_to_ptr(next_ppfn);
        }
    }

    pte = page_get_pte(page_table, level, vaddr);
    if (page_is_pte_valid(pte, level)) {
        int err = page_compare_pte_attri(pte, level, ppfn,
                                         exec, 1, write, cache, 1);
        panic_if(err, "Trying to change existing mapping!");
    } else {
        page_set_pte_attri(pte, level, ppfn, exec, 1, write, exec, 1);
    }
}

int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
              int cache, int exec, int write, int kernel, int override)
{
//     kprintf("Map range, page_table: %lx, vaddr @ %lx, paddr @ %llx, size: %lx"
//             ", cache: %d, exec: %d, write: %d, kernel: %d, override: %d\n",
//             page_table, vaddr, (u64)paddr, size,
//             cache, exec, write, 1, 0);

    atomic_mb();
    panic_if(!page_is_mappable(vaddr, paddr, exec, 1, write, cache, 1),
             "Attempting unmappable mapping!");

    ulong vaddr_start = ALIGN_DOWN((ulong)vaddr, PAGE_SIZE);
    ulong vaddr_end = ALIGN_UP((ulong)vaddr + size, PAGE_SIZE);
    u64 paddr_start = ALIGN_DOWN(paddr, PAGE_SIZE);

    int mapped_pages = 0;

    u64 cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end; ) {
        ulong block_page_count = 0;
        ulong region_size = vaddr_end - cur_vaddr;
        int block = page_get_block_level(cur_vaddr, cur_paddr, region_size,
                                         &block_page_count);

        if (!block) {
            block = PAGE_LEVELS;
            block_page_count = 1;
        }

        map_page(page_table, cur_vaddr, cur_paddr, block,
                 cache, exec, write, kernel, override);

        mapped_pages += block_page_count;
        cur_vaddr += block_page_count * PAGE_SIZE;
        cur_paddr += block_page_count * PAGE_SIZE;
    }

    atomic_mb();
    return mapped_pages;
}

int loader_map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    return map_range(page_table, vaddr, paddr, size, cache, exec, write, 1, 0);
}

#endif
