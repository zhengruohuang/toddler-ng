#ifndef __ARCH_ALPHA_COMMON_INCLUDE_PAGE_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"


#define PAGE_LEVELS             3


#define GET_L1PTE_INDEX(addr)   (((addr) >> 33) & 0x3fful)
#define GET_L2PTE_INDEX(addr)   (((addr) >> 23) & 0x3fful)
#define GET_L3PTE_INDEX(addr)   (((addr) >> 13) & 0x3fful)
#define GET_PAGE_OFFSET(addr)   ((addr) & 0x1ffful)

static inline int get_page_table_index(ulong vaddr, int level)
{
    switch (level) {
    case 0: return GET_L1PTE_INDEX(vaddr);
    case 1: return GET_L2PTE_INDEX(vaddr);
    case 2: return GET_L3PTE_INDEX(vaddr);
    default: return -1;
    }
    return -1;
}


struct page_table_entry {
    union {
        u64     value;

        struct {
            u64 valid           : 1;
            u64 no_read         : 1;
            u64 no_write        : 1;
            u64 no_exec         : 1;
            u64 global          : 1;    // matches all address spaces
            u64 granu_hint      : 2;
            u64 tlb_miss_membar : 1;
            u64 kernel_read     : 1;
            u64 user_read       : 1;
            u64 reserved1       : 2;    // reserved for HW
            u64 kernel_write    : 1;
            u64 user_write      : 1;
            u64 reserved2       : 2;    // reserved for HW
            u64 reserved3       : 16;   // reserved for SW
            u64 pfn             : 32;
        };
    };
} packed8_struct;

struct page_frame {
    union {
        u8 value_u8[8192];
        u64 value_u32[1024];
        struct page_table_entry entries[1024];
    };
} packed8_struct;


#endif
