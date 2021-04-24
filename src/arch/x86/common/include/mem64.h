// #ifndef __ARCH_X86_COMMON_INCLUDE_MEM64_H__
// #define __ARCH_X86_COMMON_INCLUDE_MEM64_H__
//
//
// #include "common/include/compiler.h"
// #include "common/include/inttypes.h"
//
//
// /*
//  * User address space
//  */
// #define USER_VADDR_SPACE_BEGIN  0x10000ul
// #define USER_VADDR_SPACE_END    0xf0000000ul    // 4GB-256MB
//
//
// /*
//  * Paging
//  *  4KB page size, 1K-entry per page, 2-level
//  */
// #define VADDR_BITS                  64
//
// #define PAGE_SIZE                   4096
// #define PAGE_BITS                   12
// #define PAGE_LEVELS                 4
//
// #define PAGE_TABLE_SIZE             4096
// #define PAGE_TABLE_ENTRY_COUNT      512
// #define PAGE_TABLE_ENTRY_BITS       9
//
// #define GET_PAGE_TABLE_ENTRY_INDEX(addr, level) (   \
//     level == 0 ? (((addr) >> 39) & 0x1fful) :       \
//     level == 1 ? (((addr) >> 30) & 0x1fful) :       \
//     level == 2 ? (((addr) >> 21) & 0x1fful) :       \
//                  (((addr) >> 12) & 0x1fful))
//
// #define GET_PML4E_INDEX(addr)       (((addr) >> 39) & 0x1fful)
// #define GET_PDPE_INDEX(addr)        (((addr) >> 30) & 0x1fful)
// #define GET_PDE_INDEX(addr)         (((addr) >> 21) & 0x1fful)
// #define GET_PTE_INDEX(addr)         (((addr) >> 12) & 0x1fful)
// #define GET_PAGE_OFFSET(addr)       ((addr) & 0xffful)
//
// struct page_table_entry {
//     union {
//         u64         value;
//
//         struct {
//             u64     present         : 1;
//             u64     write_allow     : 1;
//             u64     user_allow      : 1;
//             u64     write_thru      : 1;
//             u64     cache_disabled  : 1;
//             u64     accessed        : 1;
//             u64     dirty           : 1;    // Present in PTE only, 0 in others
//             u64     large_page      : 1;    // Present in levels other than PTE
//             u64     global          : 1;    // Present in PTE only, 0 in others
//             u64     avail           : 3;
//             u64     pfn             : 51;
//             u64     no_exec         : 1;
//         };
//
//         struct {
//             u64     reserved0       : 7;
//             u64     page_attri_pte  : 1;
//             u64     reserved1       : 4;
//             u64     page_attri_pde  : 1;
//             u64     reserved2       : 51;
//         };
//     };
// } packed_struct;
//
// struct page_frame {
//     union {
//         u8 value_u8[4096];
//         u32 value_u32[1024];
//         struct page_table_entry entries[512];
//     };
// } packed_struct;
//
//
// /*
//  * Addr <--> PFN
//  */
// #define PFN_TO_ADDR(pfn)    ((pfn) << 12)
// #define ADDR_TO_PFN(addr)   ((addr) >> 12)
//
//
// /*
//  * Segment
//  */
// struct global_desc_table_entry {
//     union {
//         u64 value;
//
//         struct {
//             u16 limit_low;
//             u16 base_low;
//             u8 base_mid;
//             union {
//                 u8 access_bytes;
//                 struct {
//                     u8 accessed     : 1;
//                     u8 read_write   : 1;    // For code: if read is allowed; For data: if write is allowed
//                     u8 direct_conform : 1;  // For data: 0 = grow up, 1 = grow down;
//                                             // For code: 0 = exec in same priv only, 1 = exec from equal or lower priv
//                     u8 exec         : 1;    // 1 = Code, 0 = Data
//                     u8 type         : 1;    // 1 = Code/Data, 0 = System
//                     u8 privilege    : 2;    // 0 = Kernel, 3 = User
//                     u8 present      : 1;
//                 };
//             };
//
//             union {
//                 u8 flags_and_limit_high;
//
//                 struct {
//                     u8 limit_high   : 4;
//                     u8 system       : 1;
//                     u8 amd64_seg    : 1;    // For 64-bit code seg only
//                     u8 size         : 1;    // 0 = 16-bit/64-bit, 1 = 32-bit
//                     u8 granularity  : 1;    // 0 = 1B, 1 = 4KB
//                 };
//             };
//
//             u8 base_high;
//         };
//     };
// } packed_struct;
//
// struct global_desc_table {
//     struct global_desc_table_entry dummy;
//     struct global_desc_table_entry code;
//     struct global_desc_table_entry data;
// } packed_struct;
//
// struct global_desc_table_ptr {
//     u16 limit;
//     u64 base;
// } packed_struct;
//
//
// #endif
