#ifndef __ARCH_MIPS_COMMON_INCLUDE_PAGE_H__
#define __ARCH_MIPS_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/mem.h"
#include "common/include/mmu.h"


/*
 * Page structure
 *  8KB page size, 2048-entry per page, 2-level
 */
#define PAGE_LEVELS             2
#define PAGE_TABLE_SIZE         8192
#define PAGE_TABLE_ENTRY_COUNT  2048
#define PAGE_TABLE_ENTRY_BITS   11

struct page_table_entry {
    union {
        struct {
            u32 coherent        : 1;
            u32 cache_disabled  : 1;
            u32 write_back      : 1;
            u32 weak_order      : 1;
            u32 accessed        : 1;
            u32 dirty           : 1;
            u32 protect_field   : 3;
            u32 has_next        : 1;
            u32 pfn             : 22;
        };

        u32 value;
    };
} packed4_struct;

struct page_frame {
    union {
        u8 bytes[PAGE_SIZE];
        struct page_table_entry entries[PAGE_TABLE_ENTRY_COUNT];
    };
} packed4_struct;


/*
 * Arch-specific page table functions
 */
static inline void *page_get_pte(void *page_table, int level, ulong vaddr)
{
    struct page_frame *table = page_table;
    ulong idx = vaddr >> PAGE_BITS;
    idx >>= (PAGE_LEVELS - level) * PAGE_TABLE_ENTRY_BITS;
    idx &= PAGE_TABLE_ENTRY_COUNT - 0x1ul;

    struct page_table_entry *pte = &table->entries[idx];
    return pte;
}

static inline int page_is_pte_valid(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return pte->protect_field ? 1 : 0;
}

static inline int page_is_pte_leaf(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return pte->has_next ? 0 : 1;
}

static inline ppfn_t page_get_pte_dest_ppfn(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return (ppfn_t)pte->pfn;
}

static inline ppfn_t page_get_pte_next_table(void *entry, int level)
{
    return page_get_pte_dest_ppfn(entry, level);
}

static inline ppfn_t page_get_pte_attri(void *entry, int level, ulong vaddr,
                                        int *exec, int *read, int *write, int *cache, int *kernel)
{
    struct page_table_entry *pte = entry;

    get_prot_attri(pte->protect_field, kernel, read, write, exec);
    if (cache) *cache = !pte->cache_disabled;

    return (ppfn_t)pte->pfn;
}

static inline int page_compare_pte_attri(void *entry, int level, paddr_t ppfn,
                                         int exec, int read, int write, int cache, int kernel)
{
    struct page_table_entry *pte = entry;

    int pte_cache = !pte->cache_disabled;
    int pte_exec, pte_read, pte_write, pte_kernel;
    get_prot_attri(pte->protect_field,
                   &pte_kernel, &pte_read, &pte_write, &pte_exec);

    if (pte->pfn != ppfn ||
        pte_exec != exec || pte_write != write ||
        pte_cache != cache || pte_kernel != kernel
    ) {
        return -1;
    }

    return 0;
}

static inline void page_set_pte_attri(void *entry, int level, paddr_t ppfn,
                                      int exec, int read, int write, int cache, int kernel)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;

    pte->protect_field = get_prot_field(kernel, read, write, exec);
    pte->has_next = 0;
    pte->pfn = ppfn;
    pte->coherent = 1;
    pte->cache_disabled = !cache;
    pte->write_back = 1;
    pte->weak_order = 1;
}

static inline void page_set_pte_next_table(void *entry, int level, ppfn_t ppfn)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;

    pte->protect_field = get_prot_field(1, 1, 1, 1);
    pte->has_next = 1;
    pte->pfn = ppfn;
}

static inline void page_zero_pte(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;
}

static inline int page_is_mappable(ulong vaddr, paddr_t paddr,
                                   int exec, int read, int write, int cache, int kernel)
{
    return 1;
}

static inline int page_is_empty_table(void *page_table, int level)
{
    struct page_frame *table = page_table;
    for (int i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
        if (table->entries[i].value) {
            return 0;
        }
    }

    return 1;
}

static inline int page_get_num_table_pages(int level)
{
    return 1;
}

static inline ulong page_get_block_page_count(int level)
{
    ulong page_count = 0x1ul << ((PAGE_LEVELS - level) * PAGE_TABLE_ENTRY_BITS);
    return page_count;
}


#endif
