#ifndef __ARCH_MIPS_COMMON_INCLUDE_PAGE64_H__
#define __ARCH_MIPS_COMMON_INCLUDE_PAGE64_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"


/*
 * Paging
 *  4KB page size, 512-entry per page, 4-level
 */
#define PAGE_TABLE_SIZE          4096
#define PAGE_TABLE_ENTRY_COUNT   512
#define PAGE_TABLE_ENTRY_BITS    9

#define PAGE_SIZE                4096
#define PAGE_BITS                12
#define PAGE_LEVELS              4

#define L0BLOCK_SIZE             0x8000000000ul  // 512GB
#define L0BLOCK_PAGE_COUNT       134217728

#define L1BLOCK_SIZE             0x40000000ul    // 1GB
#define L1BLOCK_PAGE_COUNT       262144

#define L2BLOCK_SIZE             0x200000ul      // 2MB
#define L2BLOCK_PAGE_COUNT       512

#define GET_PTE_INDEX(addr, level)  (                       \
            ((level) == 0) ? ((addr) >> 39) :               \
            ((level) == 1) ? (((addr) >> 30) & 0x1fful) :   \
            ((level) == 2) ? (((addr) >> 21) & 0x1fful) :   \
                            (((addr) >> 12) & 0x1fful)     \
        )
#define GET_L0PTE_INDEX(addr)    ((addr) >> 39)
#define GET_L1PTE_INDEX(addr)    (((addr) >> 30) & 0x1fful)
#define GET_L2PTE_INDEX(addr)    (((addr) >> 21) & 0x1fful)
#define GET_L3PTE_INDEX(addr)    (((addr) >> 12) & 0x1fful)
#define GET_PAGE_OFFSET(addr)    ((addr) & 0xffful)

struct page_table_entry {
    union {
        struct {
            ulong   present         : 1;
            ulong   block           : 1;
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

        struct {
            u32                     : 20;
            u32     bfn             : 12;
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
