#ifndef __ARCH_RISCV_COMMON_INCLUDE_MEM_H__
#define __ARCH_RISCV_COMMON_INCLUDE_MEM_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/arch.h"


/*
 * User address space
 */
#define USER_VADDR_SPACE_BEGIN  0x10000ul
#define USER_VADDR_SPACE_END    0xf0000000ul    // 4GB-256MB


/*
 * Paging
 *  4KB page size
 */
// RISC-V 64
#if (defined(ARCH_RISCV64))
#define VADDR_BITS                  48

#define PAGE_TABLE_SIZE             4096
#define PAGE_TABLE_ENTRY_COUNT      512
#define PAGE_TABLE_ENTRY_BITS       9

#if (VADDR_BITS == 39)
#   define PAGE_SIZE                4096
#   define PAGE_BITS                12
#   define PAGE_LEVELS              3

#   define L0BLOCK_SIZE             0x40000000ul    // 1GB
#   define L0BLOCK_PAGE_COUNT       262144

#   define L1BLOCK_SIZE             0x200000ul      // 2MB
#   define L1BLOCK_PAGE_COUNT       512

#   define GET_PTE_INDEX(addr, level)  (                \
        ((level) == 0) ? (((addr) >> 30) & 0x1fful) :   \
        (((level) == 1) ? (((addr) >> 21) & 0x1fful) :  \
                         (((addr) >> 12) & 0x1fful))    \
    )
#   define GET_L0PTE_INDEX(addr)    (((addr) >> 30) & 0x1fful)
#   define GET_L1PTE_INDEX(addr)    (((addr) >> 21) & 0x1fful)
#   define GET_L2PTE_INDEX(addr)    (((addr) >> 12) & 0x1fful)
#   define GET_PAGE_OFFSET(addr)    ((addr) & 0xffful)

#elif (VADDR_BITS == 48)
#   define PAGE_SIZE                4096
#   define PAGE_BITS                12
#   define PAGE_LEVELS              4

#   define L0BLOCK_SIZE             0x8000000000ul  // 512GB
#   define L0BLOCK_PAGE_COUNT       134217728

#   define L1BLOCK_SIZE             0x40000000ul    // 1GB
#   define L1BLOCK_PAGE_COUNT       262144

#   define L2BLOCK_SIZE             0x200000ul      // 2MB
#   define L2BLOCK_PAGE_COUNT       512

#   define GET_PTE_INDEX(addr, level)  (                \
        ((level) == 0) ? ((addr) >> 39) :               \
        ((level) == 1) ? (((addr) >> 30) & 0x1fful) :   \
        ((level) == 2) ? (((addr) >> 21) & 0x1fful) :   \
                         (((addr) >> 12) & 0x1fful)     \
    )
#   define GET_L0PTE_INDEX(addr)    ((addr) >> 39)
#   define GET_L1PTE_INDEX(addr)    (((addr) >> 30) & 0x1fful)
#   define GET_L2PTE_INDEX(addr)    (((addr) >> 21) & 0x1fful)
#   define GET_L3PTE_INDEX(addr)    (((addr) >> 12) & 0x1fful)
#   define GET_PAGE_OFFSET(addr)    ((addr) & 0xffful)

#else
#   error Unsupport VADDR_BITS
#endif


// RISC-V 32
#elif (defined(ARCH_RISCV32))
#define VADDR_BITS                  32

#define PAGE_SIZE                   4096
#define PAGE_BITS                   12
#define PAGE_LEVELS                 2

#define PAGE_TABLE_SIZE             4096
#define PAGE_TABLE_ENTRY_COUNT      1024
#define PAGE_TABLE_ENTRY_BITS       10

#define L0BLOCK_SIZE                0x400000ul      // 4MB
#define L0BLOCK_PAGE_COUNT          1024

#define GET_PTE_INDEX(addr, level)  (               \
    ((level) == 0) ? (((addr) >> 22) & 0x3fful) :   \
                     (((addr) >> 12) & 0x3fful)     \
)
#define GET_L0PTE_INDEX(addr)       (((addr) >> 22) & 0x3fful)
#define GET_L1PTE_INDEX(addr)       (((addr) >> 12) & 0x3fful)
#define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)

#endif


struct page_table_entry {
    union {
        ulong value;

        struct {
            ulong valid     : 1;
            ulong read      : 1;
            ulong write     : 1;
            ulong exec      : 1;
            ulong user      : 1;
            ulong global    : 1;
            ulong accessed  : 1;
            ulong dirty     : 1;
            ulong software  : 2;
            ulong pfn       : ARCH_WIDTH - 10;
        };
    };
} packed_struct;

struct page_frame {
    union {
        u8 value_u8[4096];
        u32 value_u32[1024];
        struct page_table_entry entries[PAGE_TABLE_ENTRY_COUNT];
    };
} packed_struct;


/*
 * Addr <--> PFN
 */
#define PFN_TO_ADDR(pfn)    ((pfn) << 12)
#define ADDR_TO_PFN(addr)   ((addr) >> 12)


#endif
