#ifndef __ARCH_SPARCV8_COMMON_INCLUDE_MEM_H__
#define __ARCH_SPARCV8_COMMON_INCLUDE_MEM_H__


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
#define USER_VADDR_SPACE_END    (0xf0000000ul)  // 4GB-256MB


/*
 * Paging
 *  36-bit paddr, 4KB page size, 256/64/64-entry per page, 3-level
 */
#define VADDR_BITS                  32

#define PAGE_SIZE                   4096
#define PAGE_BITS                   12
#define PAGE_LEVELS                 3

#define L1PAGE_TABLE_SIZE           1024
#define L1PAGE_TABLE_ENTRY_COUNT    256
#define L1PAGE_TABLE_ENTRY_BITS     8

#define L2PAGE_TABLE_SIZE           256
#define L2PAGE_TABLE_ENTRY_COUNT    64
#define L2PAGE_TABLE_ENTRY_BITS     6

#define L3PAGE_TABLE_SIZE           256
#define L3PAGE_TABLE_ENTRY_COUNT    64
#define L3PAGE_TABLE_ENTRY_BITS     6

#define L1BLOCK_SIZE                0x1000000ul // 16MB
#define L1BLOCK_PAGE_COUNT          4096

#define L2BLOCK_SIZE                0x40000ul   // 256KB
#define L2BLOCK_PAGE_COUNT          64

#define GET_PTE_INDEX(addr, idx)    (idx == 1 ? ((addr) >> 24) :                \
                                        (idx == 2 ? (((addr) >> 18) & 0x3ful) : \
                                            (((addr) >> 12) & 0x3ful)           \
                                    ))
#define GET_L1PTE_INDEX(addr)       ((addr) >> 24)
#define GET_L2PTE_INDEX(addr)       (((addr) >> 18) & 0x3ful)
#define GET_L3PTE_INDEX(addr)       (((addr) >> 12) & 0x3ful)
#define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)

#define PAGE_TABLE_ENTRY_TYPE_PTD   0x1
#define PAGE_TABLE_ENTRY_TYPE_PTE   0x2

// PTD and PTE structures can be combined into one. By checking the type field
// one can determine the entry type. When the entry is a PTD, certain fields
// must be 1
struct page_table_desc {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 addr_hi30       : 30;   // paddr >> 6
            u32 type            : 2;    // 0: Invalid, 1: PTD, 2: PTE, 3: Resv
        };
#endif
    };
} packed_struct;

struct page_table_entry {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 pfn             : 24;
            u32 cacheable       : 1;    // 0 when PTD
            u32 modified        : 1;    // 0 when PTD
            u32 referenced      : 1;    // 0 when PTD
            u32 perm            : 3;    // 0 when PTD
            u32 type            : 2;    // 0: Invalid, 1: PTD, 2: PTE, 3: Resv
        };
#endif
    };
} packed_struct;

struct page_frame {
    union {
        u8 value_u8[4096];
        u32 value_u32[1024];

        struct page_table_desc ptd[1024];
        struct page_table_entry pte[1024];
        struct page_table_entry entries[1024];
    };
} packed_struct;


/*
 * Addr <--> PFN
 */
#define PFN_TO_ADDR(pfn)    ((pfn) << 12)
#define ADDR_TO_PFN(addr)   ((addr) >> 12)


/*
 * Addr <--> BFN (Block Frame Number)
 */
#define L1BFN_TO_ADDR(bfn)   ((bfn) << 24)
#define ADDR_TO_L1BFN(addr)  ((addr) >> 24)

#define L2BFN_TO_ADDR(bfn)   ((bfn) << 18)
#define ADDR_TO_L2BFN(addr)  ((addr) >> 18)


#endif
