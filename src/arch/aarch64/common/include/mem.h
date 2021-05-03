#ifndef __ARCH_AARCH64_COMMON_INCLUDE_MEM_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_MEM_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


#define PAGE_SIZE               4096
#define PAGE_BITS               12
#define USE_GENERIC_PAGE_TABLE  1

typedef u64                     paddr_t;
typedef paddr_t                 ppfn_t;
typedef paddr_t                 psize_t;

#define MAX_NUM_VADDR_BITS      48                  // Must be 48 or 39
#define MAX_NUM_PADDR_BITS      52

#define USER_VADDR_BASE         0x100100000ul       // 4GB + 1MB
#define USER_VADDR_LIMIT        0x3f00000000ul
                            //  0x3f00000000ul      // 256GB - 4GB, RV39
                            //  0x7F0000000000ul    // 128TB - 1TB, RV48
                            //  0x400000000000ul




/*
 * User address space
 */
// #define USER_VADDR_SPACE_BEGIN  0x100010000ul
// #define USER_VADDR_SPACE_END    0x400000000000ul


/*
 * Paging
 *  4KB page size, 512-entry per page, 4-level
 */
// #define VADDR_BITS                  48
//
// #define PAGE_SIZE                   4096
// #define PAGE_BITS                   12
// #define PAGE_LEVELS                 4
//
// #define PAGE_TABLE_SIZE             4096
// #define PAGE_TABLE_ENTRY_SIZE       8
// #define PAGE_TABLE_ENTRY_COUNT      512
// #define PAGE_TABLE_ENTRY_BITS       9
//
// #define L1BLOCK_SIZE                0x40000000ul
// #define L1BLOCK_PAGE_COUNT          262144
//
// #define L2BLOCK_SIZE                0x200000ul
// #define L2BLOCK_PAGE_COUNT          512
//
// #define GET_PTE_INDEX(addr, level)  (((addr) >>
//                                         ((level) == 0 ? 39 :
//                                             ((level) == 1 ? 30 :
//                                                 ((level) == 2 ? 21 : 12
//                                     )))) & 0x1fful)
// #define GET_L0PTE_INDEX(addr)       (((addr) >> 39) & 0x1fful)
// #define GET_L1PTE_INDEX(addr)       (((addr) >> 30) & 0x1fful)
// #define GET_L2PTE_INDEX(addr)       (((addr) >> 21) & 0x1fful)
// #define GET_L3PTE_INDEX(addr)       (((addr) >> 12) & 0x1fful)
// #define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)
//
//
// // #define VADDR_BITS                  39
// //
// // #define PAGE_SIZE                   4096
// // #define PAGE_BITS                   12
// // #define PAGE_LEVELS                 3
// //
// // #define PAGE_TABLE_SIZE             4096
// // #define PAGE_TABLE_ENTRY_SIZE       8
// // #define PAGE_TABLE_ENTRY_COUNT      512
// // #define PAGE_TABLE_ENTRY_BITS       9
// //
// // #define GET_PTE_INDEX(addr, level)  (((addr) >>
// //                                         ((level) == 0 ? 30 :
// //                                             ((level) == 1 ? 21 : 12
// //                                     ))) & 0x1fful)
// // #define GET_L0PTE_INDEX(addr)       (((addr) >> 30) & 0x1fful)
// // #define GET_L1PTE_INDEX(addr)       (((addr) >> 21) & 0x1fful)
// // #define GET_L2PTE_INDEX(addr)       (((addr) >> 12) & 0x1fful)
// // #define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)
//
// struct page_table_entry {
//     union {
//         u64         value;
//
//         struct {
//             u64     valid           : 1;
//             u64     table           : 1;
//             u64     reserved1       : 10;
//             u64     pfn             : 36;
//             u64     reserved2       : 11;
//             u64     kernel_no_exec  : 1;
//             u64     user_no_exec    : 1;
//             u64     user            : 1;
//             u64     read_only       : 1;
//             //u64     access_perm     : 2;
//             u64     non_secure      : 1;
//         } table;
//
//         struct {
//             u64     valid           : 1;
//             u64     table           : 1;
//             u64     attrindx        : 3;
//             u64     non_secure      : 1;
//             u64     user            : 1;
//             u64     read_only       : 1;
//             //u64     access_perm     : 2;
//             u64     shareable       : 2;
//             u64     accessed        : 1;
//             u64     non_global      : 1;
//             u64     pfn             : 36;
//             u64     reserved        : 4;
//             u64     cont            : 1;
//             u64     kernel_no_exec  : 1;
//             u64     user_no_exec    : 1;
//             u64     software        : 4;
//             u64     reserved3       : 5;
//         } page;
//
//         struct {
//             u64     valid           : 1;
//             u64     table           : 1;
//             u64     attrindx        : 3;
//             u64     non_secure      : 1;
//             u64     user            : 1;
//             u64     read_only       : 1;
//             //u64     access_perm     : 2;
//             u64     shareable       : 2;
//             u64     accessed        : 1;
//             u64     non_global      : 1;
//             u64     bfn             : 36;
//             u64     reserved        : 4;
//             u64     cont            : 1;
//             u64     kernel_no_exec  : 1;
//             u64     user_no_exec    : 1;
//             u64     software        : 4;
//             u64     reserved3       : 5;
//         } block;
//     };
// } packed_struct;
//
// struct page_frame {
//     union {
//         u8 value_u8[4096];
//         u32 value_u32[1024];
//         u64 value_u64[512];
//
//         struct page_table_entry entries[PAGE_TABLE_ENTRY_COUNT];
//     };
// } packed_struct;


/*
 * Addr <--> PFN
 */
// #define PFN_TO_ADDR(pfn)    ((pfn) << 12)
// #define ADDR_TO_PFN(addr)   ((addr) >> 12)


/*
 * Addr <--> BFN (Block Frame Number)
 */
// #define L1BFN_TO_ADDR(bfn)  ((bfn) << 30)
// #define ADDR_TO_L1BFN(addr) ((addr) >> 30)
//
// #define L2BFN_TO_ADDR(bfn)  ((bfn) << 21)
// #define ADDR_TO_L2BFN(addr) ((addr) >> 21)


#endif
