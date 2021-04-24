#ifndef __ARCH_X86_COMMON_INCLUDE_PAGE_H__
#define __ARCH_X86_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/mem.h"


/*
 * Page structure
 *  4KB page size
 */
#if (defined(ARCH_IA32) && (MAX_NUM_PADDR_BITS == 32))
    #define PAGE_LEVELS             2
    #define PAGE_TABLE_SIZE         4096
    #define PAGE_TABLE_ENTRY_COUNT  1024
    #define PAGE_TABLE_ENTRY_BITS   10
    #define PAGE_PAE                0

#elif (defined(ARCH_AMD64) && (MAX_NUM_VADDR_BITS == 48))
    #define PAGE_LEVELS             4
    #define PAGE_TABLE_SIZE         4096
    #define PAGE_TABLE_ENTRY_COUNT  512
    #define PAGE_TABLE_ENTRY_BITS   9
    #define PAGE_PAE                1

#else
    #error "Unsupported arch or max paddr/vaddr width!"

#endif

#if (PAGE_PAE)
typedef u64 pte_t;
#else
typedef u32 pte_t;
#endif

struct page_table_entry {
    union {
        pte_t       value;

        struct {
            pte_t   present         : 1;
            pte_t   write_allow     : 1;
            pte_t   user_allow      : 1;
            pte_t   write_thru      : 1;
            pte_t   cache_disabled  : 1;
            pte_t   accessed        : 1;
            pte_t   dirty           : 1;    // Present in PTE only, 0 in others
            pte_t   large_page      : 1;    // Present in levels other than PTE, 0 in PTE
            pte_t   global          : 1;    // Present in PTE only, 0 in others
            pte_t   avail           : 3;
#if (PAGE_PAE)
            pte_t   pfn             : 51;
            pte_t   no_exec         : 1;
#else
            pte_t   pfn             : 20;
#endif
        };
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

#if (defined(ARCH_IA32) && (MAX_NUM_PADDR_BITS == 32))
    if (level == 1) {
        return pte->large_page ? 1 : 0;
    } else {
        return 1;
    }
#elif (defined(ARCH_AMD64) && (MAX_NUM_VADDR_BITS == 48))
    if (level < 3) {
        return 0;
    } else if (level == 3) {
        return pte->large_page ? 1 : 0;
    } else {
        return 1;
    }
#else
    #error "Unsupported arch or max paddr/vaddr width!"
#endif

    return 0;
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

#if (PAGE_PAE)
    if (exec) *exec = pte->no_exec ? 0 : 1;
#else
    if (exec) *exec = 1;
#endif
    if (read) *read = 1;
    if (write) *write = pte->write_allow ? 1 : 0;
    if (cache) *cache = pte->cache_disabled ? 0 : 1;
    if (kernel) *kernel = pte->user_allow ? 0 : 1;

    return (ppfn_t)pte->pfn;
}

static inline int page_compare_pte_attri(void *entry, int level, paddr_t ppfn,
                                         int exec, int read, int write, int cache, int kernel)
{
    struct page_table_entry *pte = entry;

    if (pte->pfn != ppfn || pte->cache_disabled != !cache ||
        pte->write_allow != write || pte->user_allow != !kernel
#if (PAGE_PAE)
        || pte->no_exec != !exec
#endif
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
    pte->pfn = ppfn;
    pte->write_allow = write;
    pte->cache_disabled = !cache;
    pte->user_allow = !kernel;
#if (defined(ARCH_IA32) && (MAX_NUM_PADDR_BITS == 32))
    pte->large_page = level == 1 ? 1 : 0;
#elif (defined(ARCH_AMD64) && (MAX_NUM_VADDR_BITS == 48))
    pte->large_page = level == 3 ? 1 : 0;
    pte->no_exec = !exec;
#else
    #error "Unsupported arch or max paddr/vaddr width!"
#endif
}

static inline void page_set_pte_next_table(void *entry, int level, ppfn_t ppfn)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;

    pte->present = 1;
    pte->pfn = ppfn;
    pte->write_allow = 1;
    pte->user_allow = 1;
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
#if (defined(ARCH_IA32) && (MAX_NUM_PADDR_BITS == 32))
    ulong page_count = 0x1ul << ((PAGE_LEVELS - level) * PAGE_TABLE_ENTRY_BITS);
    return page_count;
#elif (defined(ARCH_AMD64) && (MAX_NUM_VADDR_BITS == 48))
    if (level < 3) {
        return 0;
    } else {
        ulong page_count = 0x1ul << ((PAGE_LEVELS - level) * PAGE_TABLE_ENTRY_BITS);
        return page_count;
    }
#else
    #error "Unsupported arch or max paddr/vaddr width!"
#endif
    return 0;
}


#endif
