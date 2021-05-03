#ifndef __ARCH_AARCH64_COMMON_INCLUDE_PAGE_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/mem.h"
#include "common/include/msr.h"


/*
 * Page structure
 *  4KB page size, 512-entry per page, 4-level
 */
#define PAGE_LEVELS              4
#define PAGE_TABLE_SIZE          4096
#define PAGE_TABLE_ENTRY_COUNT   512
#define PAGE_TABLE_ENTRY_BITS    9

struct page_table_entry {
    union {
        u64 value;

        // Common fields
        struct {
            u64 present         : 1;
            u64 table           : 1;
            u64                 : 10;
            u64 pfn             : 36;
            u64                 : 16;
        };

        // Table (non-leaf) fields
        struct {
            u64                 : 2;
            u64                 : 10;
            u64                 : 36;
            u64                 : 11;
            u64 kernel_no_exec  : 1;
            u64 user_no_exec    : 1;
            u64 user            : 1;  // access_perm
            u64 read_only       : 1;  // access_perm
            u64 non_secure      : 1;
        } non_leaf;

        // Page/Block (leaf) fields
        struct {
            u64                 : 2;
            u64 attrindx        : 3;
            u64 non_secure      : 1;
            u64 user            : 1;    // access_perm
            u64 read_only       : 1;    // access_perm
            u64 shareable       : 2;
            u64 accessed        : 1;
            u64 non_global      : 1;
            u64                 : 36;
            u64                 : 4;
            u64 contiguous      : 1;
            u64 kernel_no_exec  : 1;
            u64 user_no_exec    : 1;
            u64 software        : 4;
            u64                 : 5;
        } leaf;
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
    return level == 4 ? 1 : (pte->table ? 0 : 1);
}

static inline ppfn_t page_get_pte_next_table(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    return (ppfn_t)pte->pfn;
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

    if (exec) *exec = pte->leaf.user ? (pte->leaf.user_no_exec ? 0 : 1) :
                                       (pte->leaf.kernel_no_exec ? 0 : 1);
    if (read) *read = 1;
    if (write) *write = pte->leaf.read_only ? 0 : 1;
    if (cache) *cache = pte->leaf.attrindx == MAIR_IDX_NORMAL ? 1 : 0;
    if (kernel) *kernel = pte->leaf.user ? 0 : 1;

    return (ppfn_t)pte->pfn;
}

static inline int page_compare_pte_attri(void *entry, int level, paddr_t ppfn,
                                         int exec, int read, int write, int cache, int kernel)
{
    struct page_table_entry *pte = entry;

    int expect_attrindx = cache ? MAIR_IDX_NORMAL : MAIR_IDX_DEVICE;
    int expect_user = kernel ? 0 : 1;
    int expect_read_only = write ? 0 : 1;
    int expect_shareable = cache ? 0x3 : 0x2;
    int expect_kernel_no_exec = kernel ? (exec ? 0 : 1) : 0;
    int expect_user_no_exec = kernel ? 1 : (exec ? 0 : 1);

    if (pte->pfn != ppfn ||
        pte->leaf.attrindx != expect_attrindx ||
        pte->leaf.user != expect_user ||
        pte->leaf.read_only != expect_read_only ||
        pte->leaf.shareable != expect_shareable ||
        pte->leaf.kernel_no_exec != expect_kernel_no_exec ||
        pte->leaf.user_no_exec != expect_user_no_exec
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

    if (level == 2) {
        ppfn >>= 18;
    } else if (level == 3) {
        ppfn >>= 9;
    }

    pte->present = 1;
    pte->table = level == 4 ? 1 : 0;
    pte->pfn = ppfn;
    pte->leaf.attrindx = cache ? MAIR_IDX_NORMAL : MAIR_IDX_DEVICE;
//     pte->leaf.non_secure = 1;
    pte->leaf.user = kernel ? 0 : 1;
    pte->leaf.read_only = 0; // write ? 0 : 1;
    pte->leaf.shareable = cache ? 0x3 : 0x2;
    pte->leaf.accessed = 1;
    pte->leaf.non_global = 1;
    pte->leaf.kernel_no_exec = exec ? 0 : 1; // kernel ? (exec ? 0 : 1) : 0;
    pte->leaf.user_no_exec = exec ? 0 : 1; // kernel ? 1 : (exec ? 0 : 1);
}

static inline void page_set_pte_next_table(void *entry, int level, ppfn_t ppfn)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;

    pte->present = 1;
    pte->table = 1;
    pte->pfn = ppfn;
    pte->non_leaf.kernel_no_exec = 0;
    pte->non_leaf.user_no_exec = 0;
    pte->non_leaf.read_only = 0;
//     pte->non_leaf.user = 1;
//     pte->non_leaf.non_secure = 1;
}

static inline void page_zero_pte(void *entry, int level)
{
    struct page_table_entry *pte = entry;
    pte->value = 0;
}

static inline int page_is_mappable(ulong vaddr, paddr_t paddr,
                                   int exec, int read, int write, int cache, int kernel)
{
    if (vaddr >> 63) {
        if ((vaddr & 0xffff800000000000ul) != 0xffff800000000000ul) {
            return 0;
        }
    } else {
        if ((vaddr & 0xffff800000000000ul) != 0) {
            return 0;
        }
    }

    return paddr < ((paddr_t)0x1ul << MAX_NUM_PADDR_BITS) ? 1 : 0;
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
//     return 0;
//
    if (level <= 1) {
        return 0;
    }

    ulong page_count = 0x1ul << ((PAGE_LEVELS - level) * PAGE_TABLE_ENTRY_BITS);
    return page_count;
}


#endif
