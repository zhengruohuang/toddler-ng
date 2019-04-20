#ifndef __ARCH_POWERPC_COMMON_INCLUDE_MEM__
#define __ARCH_POWERPC_COMMON_INCLUDE_MEM__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * Supported memory range
 */
#define PHYS_MEM_RANGE_MIN      (0x0)
#define PHYS_MEM_RANGE_MAX      (0xf0000000ul)  // 4GB-256MB


/*
 * User address space
 */
#define USER_VADDR_SPACE_BEGIN  (0x10000ul)
#define USER_VADDR_SPACE_END    (0xf0000000ul)  // 4GB-256MB, the last segment is for kernel


/*
 * Paging
 *  4KB page size, 1K-entry per page, 2-level
 */
#define VADDR_BITS              32

#define PAGE_SIZE               4096
#define PAGE_BITS               12
#define PAGE_LEVELS             2

#define PAGE_TABLE_SIZE         4096
#define PAGE_TABLE_ENTRY_COUNT  1024
#define PAGE_TABLE_ENTRY_BITS   10

#define L0BLOCK_SIZE            0x400000ul
#define L0BLOCK_PAGE_COUNT      1024

#define GET_PTE_INDEX(addr, level)  ((addr) >> (level == 0 ? 22 : 12))
#define GET_L0PTE_INDEX(addr)       ((addr) >> 22)
#define GET_L1PTE_INDEX(addr)       (((addr) >> 12) & 0x3fful)
#define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)

struct page_table_entry {
    union {
        struct {
            u32     present         : 1;
            u32     read_allow      : 1;
            u32     write_allow     : 1;
            u32     exec_allow      : 1;
            u32     cache_allow     : 1;
            u32     kernel          : 1;
            u32     next_level      : 1;
            u32     reserved        : 5;
            u32     pfn             : 20;
        };

        u32 value;
    };
} packed_struct;

struct page_frame {
    union {
        u8 bytes[PAGE_SIZE];
        struct page_table_entry entries[PAGE_TABLE_ENTRY_COUNT];
    };
} packed_struct;


/*
 * Addr <--> PFN
 */
#define PFN_TO_ADDR(pfn)    ((pfn) << 12)
#define ADDR_TO_PFN(addr)   ((addr) >> 12)


/*
 * Page hash table
 */
struct pht_entry {
    union {
        u32 word0;

        struct {
            u32     valid       : 1;
            u32     vsid        : 24;
            u32     secondary   : 1;
            u32     page_idx    : 6;
        };
    };

    union {
        u32 word1;

        struct {
            u32     pfn         : 20;
            u32     pfn_ext     : 3;    // For 36-bit physical address
            u32     referenced  : 1;
            u32     changed     : 1;
            u32     write_thru  : 1;
            u32     no_cache    : 1;
            u32     coherent    : 1;
            u32     guarded     : 1;
            u32     pfn_ext2    : 1;
            u32     protect     : 2;
        };
    };
} packed_struct;

struct pht_group {
    struct pht_entry entries[8];
} packed_struct;


/*
 * OS PHT attribute table
 */
struct pht_attri_entry {
    union {
        u8 value;
        struct {
            u8 persist      : 1;
            u8 temporary    : 1;
            u8 reserved     : 6;
        };
    };
} packed_struct;

struct pht_attri_group {
    struct pht_attri_entry entries[8];
} packed_struct;


/*
 * Segment register
 */
struct seg_reg {
    union {
        struct {
            u32     direct_store    : 1;
            u32     key_kernel      : 1;
            u32     key_user        : 1;
            u32     no_exec         : 1;
            u32     reserved        : 4;
            u32     vsid            : 24;
        };

        u32 value;
    };
} packed_struct;


#endif
