#ifndef __ARCH_MIPS_COMMON_INCLUDE_MEM__
#define __ARCH_MIPS_COMMON_INCLUDE_MEM__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * Segment
 */
#define SEG_USER            0x0ul
#define SEG_LOW_CACHED      0x80000000ul
#define SEG_LOW_DIRECT      0xa0000000ul
#define SEG_KERNEL          0xc0000000ul


/*
 * Physical memory access window
 */
#define SEG_ACC_CACHED      0x80000000ul
#define SEG_ACC_DIRECT      0xa0000000ul
#define SEG_ACC_SIZE        0x20000000ul
#define SEG_ACC_MASK        0x1ffffffful


/*
 * User address space
 */
#define USER_VADDR_SPACE_BEGIN  0x10000ul
#define USER_VADDR_SPACE_END    0x7f000000ul


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

#define GET_PDE_INDEX(addr)     ((addr) >> 22)
#define GET_PTE_INDEX(addr)     (((addr) >> 12) & 0x3fful)
#define GET_PAGE_OFFSET(addr)   ((addr) & 0xffful)

struct page_table_entry {
    union {
        struct {
            u32     present         : 1;
            u32     read_allow      : 1;
            u32     write_allow     : 1;
            u32     exec_allow      : 1;
            u32     cache_allow     : 1;
            u32     reserved        : 7;
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
 * Physical addr <--> Code access window addr (ACC_CODE)
 */
#define PHYS_TO_ACC_CODE(addr)  ((addr) | SEG_LOW_CACHED)
#define ACC_CODE_TO_PHYS(addr)  ((addr) & SEG_ACC_MASK)


/*
 * Physical addr <--> Data access window addr (ACC_DATA)
 */
#define PHYS_TO_ACC_DATA(addr)  ((addr) | SEG_ACC_CACHED)
#define ACC_DATA_TO_PHYS(addr)  ((addr) & SEG_ACC_MASK)


#endif
