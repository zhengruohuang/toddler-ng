#ifndef __ARCH_MIPS_COMMON_INCLUDE_PAGE_H__
#define __ARCH_MIPS_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"


/*
 * Paging
 *  4KB page size, 1K-entry per page, 2-level
 */
#define PAGE_LEVELS             2

#define PAGE_TABLE_SIZE         4096
#define PAGE_TABLE_ENTRY_COUNT  1024
#define PAGE_TABLE_ENTRY_BITS   10

#define L1BLOCK_SIZE            0x400000ul
#define L1BLOCK_PAGE_COUNT      1024

#define GET_PDE_INDEX(addr)     ((addr) >> 22)
#define GET_PTE_INDEX(addr)     (((addr) >> 12) & 0x3fful)
#define GET_PAGE_OFFSET(addr)   ((addr) & 0xffful)

struct page_table_entry {
    union {
        struct {
            u32     present         : 1;
            u32     block           : 1;
            u32     global          : 1;
            u32     read_allow      : 1;
            u32     write_allow     : 1;
            u32     exec_allow      : 1;
            u32     cache_allow     : 1;
            u32     reserved        : 5;
            u32     pfn             : 20;
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
 * Addr <--> BFN (Block Frame Number)
 */
static inline ppfn_t paddr_to_pbfn(paddr_t paddr)
{
    return paddr >> 20;
}

static inline ppfn_t pbfn_to_paddr(paddr_t ppfn)
{
    return ppfn << 20;
}

static inline ulong get_pblock_offset(paddr_t paddr)
{
    return (ulong)(paddr & ((paddr_t)L1BLOCK_SIZE - 1));
}

static inline ulong get_vblock_offset(ulong vaddr)
{
    return vaddr & ((ulong)L1BLOCK_SIZE - 1);
}


#endif
