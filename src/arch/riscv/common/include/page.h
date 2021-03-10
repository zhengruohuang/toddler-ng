#ifndef __ARCH_RISCV_COMMON_INCLUDE_PAGE_H__
#define __ARCH_RISCV_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/mem.h"


/*
 * Page structure
 *  4KB page size
 */
#if (defined(ARCH_RISCV64) && (MAX_NUM_VADDR_BITS == 48))
    #define PAGE_LEVELS             4
    #define PAGE_TABLE_SIZE         4096
    #define PAGE_TABLE_ENTRY_COUNT  512
    #define PAGE_TABLE_ENTRY_BITS   9

#elif (defined(ARCH_RISCV64) && (MAX_NUM_VADDR_BITS == 39))
    #define PAGE_LEVELS             3
    #define PAGE_TABLE_SIZE         4096
    #define PAGE_TABLE_ENTRY_COUNT  512
    #define PAGE_TABLE_ENTRY_BITS   9

#elif (defined(ARCH_RISCV32) && (MAX_NUM_VADDR_BITS == 32))
    #define PAGE_LEVELS             2
    #define PAGE_TABLE_SIZE         4096
    #define PAGE_TABLE_ENTRY_COUNT  1024
    #define PAGE_TABLE_ENTRY_BITS   10

#else
    #error "Unsupported arch or max vaddr width!"

#endif

struct page_table_entry {
    union {
        ulong value;

        struct {
            ulong valid     : 1;
            ulong read      : 1;
            ulong write     : 1;
            ulong exec      : 1;
            ulong user      : 1;
            ulong global    : 1;
            ulong accessed  : 1;
            ulong dirty     : 1;
            ulong leaf      : 1;    // Part of software bits
            ulong reserved  : 1;    // Part of software bits
            ulong pfn       : sizeof(ulong) * 8 - 10;
        };
    };
} packed_struct;

struct page_frame {
    union {
        u8 bytes[PAGE_SIZE];
        struct page_table_entry entries[PAGE_TABLE_ENTRY_COUNT];
    };
} packed_struct;


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
    return pte->valid ? 1 : 0;
}

static inline int page_is_pte_leaf(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return pte->leaf ? 1 : 0;
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

    if (exec) *exec = pte->exec ? 1 : 0;
    if (read) *read = pte->read ? 1 : 0;
    if (write) *write = pte->write ? 1 : 0;
    if (cache) *cache = 1;
    if (kernel) *kernel = pte->user ? 0 : 1;

    return (ppfn_t)pte->pfn;
}

static inline int page_compare_pte_attri(void *entry, int level, paddr_t ppfn,
                                         int exec, int read, int write, int cache, int kernel)
{
    struct page_table_entry *pte = entry;

    if (pte->pfn != ppfn ||
        pte->exec != exec || pte->read != read ||
        pte->write != write || pte->user != !kernel
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

    pte->valid = 1;
    pte->read = read;
    pte->write = write;
    pte->exec = exec;
    pte->user = !kernel;
    pte->global = 0; // TODO
    pte->accessed = 0;
    pte->dirty = 0;
    pte->leaf = 1;
    pte->pfn = ppfn;
}

static inline void page_set_pte_next_table(void *entry, int level, ppfn_t ppfn)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;

    pte->valid = 1;
    //pte->read = 1;
    //pte->write = 1;
    //pte->exec = 1;
    pte->user = 0;  // TODO
    pte->global = 0; // TODO
    pte->accessed = 0;
    pte->dirty = 0;
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
