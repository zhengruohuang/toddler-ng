#ifndef __ARCH_ARM_COMMON_INCLUDE_PAGE_H__
#define __ARCH_ARM_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"


/*
 * Paging
 *  4KB page size, 1K-entry per page, 2-level
 *
 * AP[2:0]  Kernel  User
 * 000      x       x
 * 001      rw      x
 * 010      rw      r
 * 011      rw      rw
 * 100      -       -
 * 101      r       x
 * 110      r       r
 * 111      r       r
 *
 * TEX[2] == 1 -> Cacheable
 * TEX[1:0]/CB  Attri
 * 00           Non-cacheable
 * 01           Write-back, write-allocate
 * 10           Write-through, no write-allocate
 * 11           Write-back, no write-allocate
 *
 * TEX[2] == 0 -> Special
 * TEX[1:0]:C:B Type            Shareable       Desc
 * 00:0:0       Strong-ordered  Shareable       Strongly-ordered
 * 00:0:1       Device          Shareable       Shareable device
 * 00:1:0       Normal          S bit           Outer and inner write-through, no write-allocate
 * 00:1:1       Normal          S bit           Outer and inner write-back, no write-allocate
 * 01:0:0       Normal          S bit           Outer and inner non-cacheable
 * 01:0:1       -               -               Reserved
 * 01:1:0       Impl            Impl            Implementation-defined
 * 01:1:1       Normal          S bit           Outer and inner write-back, write-allocate
 * 10:0:0       Device          Non-shareable   Non-shareable device
 * 10:0:1       -               -               Reserved
 * 10:1:X       -               -               Reserved
 * 11:X:X       -               -               Reserved
 */
#define PAGE_LEVELS                 2

#define L1PAGE_TABLE_SIZE           16384
#define L1PAGE_TABLE_ENTRY_COUNT    4096
#define L1PAGE_TABLE_ENTRY_BITS     12
#define L1PAGE_TABLE_SIZE_IN_PAGES  4

#define L2PAGE_TABLE_SIZE           4096
#define L2PAGE_TABLE_ENTRY_COUNT    256
#define L2PAGE_TABLE_ENTRY_BITS     8
#define L2PAGE_TABLE_SIZE_IN_PAGES  1

#define L1BLOCK_SIZE                0x100000ul
#define L1BLOCK_PAGE_COUNT          256

#define GET_L1PTE_INDEX(addr)       ((addr) >> 20)
#define GET_L2PTE_INDEX(addr)       (((addr) >> 12) & 0xfful)
#define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)

struct l1page_table_entry {
    union {
        u32         value;

        struct {
            u32     present         : 1;
            u32     reserved1       : 2;
            u32     non_secure      : 1;
            u32     imp             : 1;
            u32     domain          : 4;
            u32     reserved3       : 1;
            u32     pfn_ext         : 2;
            u32     pfn             : 20;
        } pte;

        struct {
            u32     reserved1       : 1;
            u32     present         : 1;
            u32     cache_inner     : 2;    // c, b
            u32     no_exec         : 1;
            u32     domain          : 4;
            u32     imp             : 1;
            u32     user_write      : 1;    // AP[0]
            u32     user_access     : 1;    // AP[1]
            u32     cache_outer     : 2;    // TEX[1:0]
            u32     cacheable       : 1;    // TEX[2]
            u32     read_only       : 1;    // AP[2]
            u32     shareable       : 1;
            u32     non_global      : 1;
            u32     reserved3       : 1;
            u32     non_secure      : 1;
            u32     bfn             : 12;
        } block;
    };
} packed4_struct;

struct l2page_table_entry {
    union {
        u32         value;

        struct {
            u32     no_exec         : 1;
            u32     present         : 1;
            u32     cache_inner     : 2;    // c, b
            u32     user_write      : 1;    // AP[0]
            u32     user_access     : 1;    // AP[1]
            u32     cache_outer     : 2;    // TEX[1:0]
            u32     cacheable       : 1;    // TEX[2]
            u32     read_only       : 1;    // AP[2]    for both user and kernel
            u32     shareable       : 1;
            u32     non_global      : 1;
            u32     pfn             : 20;
        };
    };
} packed4_struct;

struct page_frame {
    union {
        u8 value_u8[4096];
        u32 value_u32[1024];
    };
} packed4_struct;

struct l1table {
    union {
        u8 value_u8[16384];
        u32 value_u32[4096];
        struct page_frame pages[4];

        struct l1page_table_entry entries[L1PAGE_TABLE_ENTRY_COUNT];
    };
} packed4_struct;

struct l2table {
    union {
        u8 value_u8[4096];
        u32 value_u32[1024];
        struct page_frame page;

        struct {
            struct l2page_table_entry entries[256];
            struct l2page_table_entry entries_dup[3][256];
        };
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
