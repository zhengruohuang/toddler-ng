#ifndef __ARCH_X86_COMMON_INCLUDE_MEM32_H__
#define __ARCH_X86_COMMON_INCLUDE_MEM32_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * User address space
 */
#define USER_VADDR_SPACE_BEGIN  0x10000ul
#define USER_VADDR_SPACE_END    0xf0000000ul    // 4GB-256MB


/*
 * Paging
 *  4KB page size, 1K-entry per page, 2-level
 */
#define VADDR_BITS                  32

#define PAGE_SIZE                   4096
#define PAGE_BITS                   12
#define PAGE_LEVELS                 2

#define PAGE_TABLE_SIZE             4096
#define PAGE_TABLE_ENTRY_COUNT      1024
#define PAGE_TABLE_ENTRY_BITS       10

#define GET_PDE_INDEX(addr)         ((addr) >> 22)
#define GET_PTE_INDEX(addr)         (((addr) >> 12) & 0x3fful)
#define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)

struct page_table_entry {
    union {
        u32         value;

        struct {
            u32     present         : 1;
            u32     write_allow     : 1;
            u32     user_allow      : 1;
            u32     write_thru      : 1;
            u32     cache_disabled  : 1;
            u32     accessed        : 1;
            u32     dirty           : 1;    // Present in PTE only, 0 in PDE
            u32     large_page      : 1;    // Present in PDE only, 0 in PTE
            u32     global          : 1;    // Present in PTE only, ignored in PDE
            u32     avail           : 3;
            u32     pfn             : 20;
        };
    };
} packed_struct;

struct page_frame {
    union {
        u8 value_u8[4096];
        u32 value_u32[1024];
        struct page_table_entry entries[1024];
    };
} packed_struct;


/*
 * Addr <--> PFN
 */
#define PFN_TO_ADDR(pfn)    ((pfn) << 12)
#define ADDR_TO_PFN(addr)   ((addr) >> 12)


#endif
