#ifndef __ARCH_MIPS_COMMON_INCLUDE_PAGE_H__
#define __ARCH_MIPS_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/mem.h"


/*
 * Page structure
 */
#if (ARCH_WIDTH == 32)
    /*
     *  4KB page size, 1024-entry per page, 2-level
     */
    #define PAGE_LEVELS             2
    #define PAGE_TABLE_SIZE         4096
    #define PAGE_TABLE_ENTRY_COUNT  1024
    #define PAGE_TABLE_ENTRY_BITS   10
#elif (ARCH_WIDTH == 64)
    /*
     * 4KB page size, 512-entry per page, 4-level
     */
    #define PAGE_LEVELS              4
    #define PAGE_TABLE_SIZE          4096
    #define PAGE_TABLE_ENTRY_COUNT   512
    #define PAGE_TABLE_ENTRY_BITS    9
#else
    #error "Unsupported arch width"
#endif

struct page_table_entry {
    union {
        struct {
            ulong   present         : 1;
            ulong   leaf            : 1;
            ulong   global          : 1;
            ulong   read_allow      : 1;
            ulong   write_allow     : 1;
            ulong   exec_allow      : 1;
            ulong   cache_allow     : 1;
            ulong   kernel          : 1;
            ulong   reserved        : 4;
#if (ARCH_WIDTH == 32)
            ulong   pfn             : 20;
#elif (ARCH_WIDTH == 64)
            ulong   pfn             : 52;
#endif
        };

        ulong value;
    };
} natural_struct;

struct page_frame {
    union {
        u8 bytes[PAGE_SIZE];
        struct page_table_entry entries[PAGE_TABLE_ENTRY_COUNT];
    };
} natural_struct;


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
    return pte->present ? 1 : 0;
}

static inline int page_is_pte_leaf(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return pte->leaf ? 1 : 0;
}

static inline ppfn_t page_get_pte_next_table(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return pte->pfn;
}

static inline ppfn_t page_get_pte_dest_ppfn(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return (ppfn_t)pte->pfn;
}

static inline ppfn_t page_get_pte_attri(void *entry, int level, ulong vaddr,
                                        int *exec, int *read, int *write, int *cache, int *kernel)
{
    struct page_table_entry *pte = entry;

    if (exec) *exec = pte->exec_allow;
    if (read) *read = 1;
    if (write) *write = pte->write_allow;
    if (cache) *cache = pte->cache_allow;
    if (kernel) *kernel = pte->kernel;

    return (ppfn_t)pte->pfn;
}

static inline int page_compare_pte_attri(void *entry, int level, paddr_t ppfn,
                                         int exec, int read, int write, int cache, int kernel)
{
    struct page_table_entry *pte = entry;

    if (pte->pfn != ppfn ||
        pte->exec_allow != exec || pte->write_allow != write ||
        pte->cache_allow != cache || pte->kernel != kernel
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

    pte->present = 1;
    pte->leaf = 1;
    pte->pfn = ppfn;
    pte->exec_allow = exec;
    pte->write_allow = write;
    pte->cache_allow = cache;
    pte->kernel = kernel;
}

static inline void page_set_pte_next_table(void *entry, int level, ppfn_t ppfn)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;

    pte->present = 1;
    pte->leaf = 0;
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
#if (ARCH_WIDTH == 32)
    return 1;
#elif (ARCH_WIDTH == 64)
    return paddr < ((paddr_t)0x1ul << MAX_NUM_PADDR_BITS) ? 1 : 0;
#endif
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
