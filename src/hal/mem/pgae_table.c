#include "common/include/abi.h"

#if 0


#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/page.h"
#include "common/include/abi.h"
#include "hal/include/lib.h"
#include "hal/include/kernel.h"
#include "hal/include/hal.h"
#include "hal/include/mem.h"
#include "hal/include/setup.h"


/*
 * Find out translation block size
 */
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

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    if (funcs->has_direct_access) {
        ptr = (void *)funcs->direct_paddr_to_vaddr(paddr, 0, 1);
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
 * Alloc and free
 */
static inline void *page_alloc_table(palloc_t palloc, int level, ppfn_t *ppfn)
{
    int num_pages = page_get_num_table_pages(level);
    ppfn_t pfn = palloc(num_pages);
    if (ppfn) {
        *ppfn = pfn;
    }

    void *ptr = page_ppfn_to_ptr(pfn);
    memzero(ptr, num_pages * PAGE_SIZE);

    return ptr;
}

static inline void page_free_table(void *page_table, int level, ppfn_t ppfn)
{
    int num_freed_pages = kernel_pfree(ppfn);
    int num_table_pages = page_get_num_table_pages(level);
    assert(num_freed_pages == num_table_pages);
}


/*
 * Page table
 */
#if (defined(ARCH_MIPS))
void *init_user_page_table()
{
    int num_pages = page_get_num_table_pages(1);
    void *page_table = kernel_palloc_ptr(num_pages);
    memzero(page_table, num_pages * PAGE_SIZE);
    return page_table;
}

void free_user_page_table(void *ptr)
{
    kernel_pfree_ptr(ptr);
}
#endif


/*
 * Get physical address of a user virtual address
 */
static paddr_t _translate_attri(void *page_table, ulong vaddr, int need_attri,
                                int *exec, int *read, int *write, int *cache, int *kernel)
{
//     kprintf("Translate, page_table: %lx, vaddr @ %lx\n", page_table, vaddr);

    atomic_mb();

    int level = 0;
    void *pte = NULL;
    paddr_t paddr = 0;

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

// TODO: add kernel
paddr_t translate_attri(void *page_table, ulong vaddr,
                        int *exec, int *read, int *write, int *cache)
{
    return _translate_attri(page_table, vaddr, 1, exec, read, write, cache, NULL);
}

paddr_t translate_attri2(void *page_table, ulong vaddr,
                         int *exec, int *read, int *write, int *cache, int *kernel)
{
    return _translate_attri(page_table, vaddr, 1, exec, read, write, cache, kernel);
}

paddr_t translate(void *page_table, ulong vaddr)
{
    return _translate_attri(page_table, vaddr, 0, NULL, NULL, NULL, NULL, NULL);
}


/*
 * Map
 */
static void map_page(void *page_table, ulong vaddr, paddr_t paddr, int block,
                     int cache, int exec, int write, int kernel, int override,
                     palloc_t palloc)
{
//     kprintf("  Map page, page_table: %lx, vaddr @ %lx, paddr @ %llx, block: %d"
//             ", cache: %d, exec: %d, write: %d, kernel: %d, override: %d\n",
//             page_table, vaddr, (u64)paddr, block,
//             cache, exec, write, kernel, override);

    ppfn_t ppfn = paddr_to_ppfn(paddr);
    int level = 0;
    void *pte = NULL;

    for (level = 1; level < block; level++) {
        pte = page_get_pte(page_table, level, vaddr);

        if (!page_is_pte_valid(pte, level)) {
            ppfn_t next_ppfn = 0;
            page_table = page_alloc_table(palloc, level + 1, &next_ppfn);
            assert(next_ppfn);
            assert(page_table);
            page_set_pte_next_table(pte, level, next_ppfn);
        } else {
            if (page_is_pte_leaf(pte, level)) {
                int err = page_compare_pte_attri(pte, level, ppfn,
                                                 exec, 1, write, cache, kernel);
                panic_if(err, "Incompatible mapping!");
            }

            ppfn_t next_ppfn = page_get_pte_next_table(pte, level);
            page_table = page_ppfn_to_ptr(next_ppfn);
        }
    }

    pte = page_get_pte(page_table, level, vaddr);
    if (page_is_pte_valid(pte, level)) {
        int err = page_compare_pte_attri(pte, level, ppfn,
                                         exec, 1, write, cache, kernel);
        if (err) {
            int ori_e, ori_w, ori_c, ori_k;
            translate_attri2(page_table, vaddr, &ori_e, NULL, &ori_w, &ori_c, &ori_k);
            panic("Trying to change existing mapping @ %lx -> "
                  "ori @ %llx (e: %d, w: %d, c: %d, k: %d), "
                  "new @ %llx (e: %d, w: %d, c: %d, k: %d)\n",
                  vaddr,
                  (u64)ppfn_to_paddr(ppfn), ori_e, ori_w, ori_c, ori_k,
                  (u64)paddr, exec, write, cache, kernel);
        }

    } else {
        page_set_pte_attri(pte, level, ppfn, exec, 1, write, exec, kernel);
    }
}

int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
              int cache, int exec, int write, int kernel, int override,
              palloc_t palloc)
{
//     kprintf("Map range, page_table: %lx, vaddr @ %lx, paddr @ %llx, size: %lx"
//             ", cache: %d, exec: %d, write: %d, kernel: %d, override: %d\n",
//             page_table, vaddr, (u64)paddr, size,
//             cache, exec, write, kernel, override);

    atomic_mb();
    panic_if(!page_is_mappable(vaddr, paddr, exec, 1, write, cache, kernel),
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
                 cache, exec, write, kernel, override, palloc);

        mapped_pages += block_page_count;
        cur_vaddr += block_page_count * PAGE_SIZE;
        cur_paddr += block_page_count * PAGE_SIZE;
    }

    atomic_mb();
    return mapped_pages;
}


/*
 * Unmap
 */
static void unmap_page(void *page_table, ulong vaddr, paddr_t paddr, int block)
{
    //kprintf("unmap page, vaddr @ %lx, paddr @ %llx\n", vaddr, (u64)paddr);

    void *page_tables[PAGE_LEVELS + 1] = { [0] = NULL, [1] = page_table };
    void *ptes[PAGE_LEVELS] = { [0] = NULL };

    void *pte = NULL;
    int level = 0;

    // Fast forward to the target level
    for (level = 1; level < block; level++) {
        pte = page_get_pte(page_table, level, vaddr);
        ptes[level] = pte;
        panic_if(!page_is_pte_valid(pte, level), "Bad mapping!");
        panic_if(page_is_pte_leaf(pte, level), "Bad mapping!");

        ppfn_t next_ppfn = page_get_pte_next_table(pte, level);
        page_table = page_ppfn_to_ptr(next_ppfn);
        page_tables[level + 1] = page_table;
    }

    // Unmap target PTE
    pte = page_get_pte(page_table, level, vaddr);
    panic_if(!page_is_pte_valid(pte, level), "Bad mapping!");
    panic_if(!page_is_pte_leaf(pte, level), "Bad mapping!");

    page_zero_pte(pte, level);

    // Free intermediate page tables
    for (; level > 1; level--) {
        page_table = page_tables[level];
        if (!page_is_empty_table(page_table, level)) {
            break;
        }

        int prev_level = level - 1;
        void *prev_pte = ptes[prev_level];

        ppfn_t page_table_ppfn = page_get_pte_next_table(prev_pte, prev_level);
        void *page_table_ptr = page_ppfn_to_ptr(page_table_ppfn);
        assert(page_table_ptr == page_table);

        page_free_table(page_table, level, page_table_ppfn);
        page_zero_pte(prev_pte, prev_level);
    }
}

int unmap_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size)
{
    //kprintf("To unmap, pfn: %u, vaddr: %u, paddr: %u, size: %u\n",
    //        page_dir_pfn, vaddr, paddr, length);

    ulong vaddr_start = align_down_vaddr(vaddr, PAGE_SIZE);
    paddr_t paddr_start = align_down_paddr(paddr, PAGE_SIZE);
    ulong vaddr_end = align_up_vaddr(vaddr + size, PAGE_SIZE);

    int unmapped_pages = 0;

    paddr_t cur_paddr = paddr_start;
    for (ulong cur_vaddr = vaddr_start; cur_vaddr < vaddr_end;) {
        ulong block_page_count = 0;
        ulong region_size = vaddr_end - cur_vaddr;
        int block = page_get_block_level(cur_vaddr, cur_paddr, region_size,
                                         &block_page_count);

        if (!block) {
            block = PAGE_LEVELS;
            block_page_count = 1;
        }

        unmap_page(page_table, cur_vaddr, cur_paddr, block);

        unmapped_pages += block_page_count;
        cur_vaddr += block_page_count * PAGE_SIZE;
        cur_paddr += block_page_count * PAGE_SIZE;
    }

    return unmapped_pages;
}

#endif
