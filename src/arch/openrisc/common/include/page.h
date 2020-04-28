#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_PAGE_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_PAGE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"


/*
 * User address space
 */
// #define USER_VADDR_SPACE_BEGIN  0x10000ul
// #define USER_VADDR_SPACE_END    0x7f000000ul


/*
 * Paging
 *  8KB page size, 256-entry per L1 table, 2048-entry per L2 table, 2-level
 */

#define PAGE_LEVELS                 2

#define L1PAGE_TABLE_SIZE           1024
#define L1PAGE_TABLE_ENTRY_COUNT    256
#define L1PAGE_TABLE_ENTRY_BITS     8
#define L1PAGE_TABLE_SIZE_IN_PAGES  8

#define L2PAGE_TABLE_SIZE           8192
#define L2PAGE_TABLE_ENTRY_COUNT    2048
#define L2PAGE_TABLE_ENTRY_BITS     11
#define L2PAGE_TABLE_SIZE_IN_PAGES  1

#define L1BLOCK_SIZE                0x1000000ul
#define L1BLOCK_PAGE_COUNT          2048

#define GET_L1PTE_INDEX(addr)       ((addr) >> 24)
#define GET_L2PTE_INDEX(addr)       (((addr) >> 13) & 0x1fful)
#define GET_PAGE_OFFSET(addr)       ((addr) & 0x1ffful)

struct page_table_entry {
    union {
        struct {
            u32 coherent        : 1;
            u32 cache_disabled  : 1;
            u32 write_back      : 1;
            u32 weak_order      : 1;
            u32 accessed        : 1;
            u32 dirty           : 1;
            u32 protect_field   : 3;
            u32 has_next        : 1;
            u32 pfn             : 22;
        };

        u32 value;
    };
} packed4_struct;

struct page_frame {
    u8 bytes[PAGE_SIZE];
} packed4_struct;

struct l1table {
    union {
        u8 bytes[PAGE_SIZE];
        struct page_frame page;

        struct {
            struct page_table_entry entries[L1PAGE_TABLE_ENTRY_COUNT];
            struct page_table_entry entries_dup[L1PAGE_TABLE_SIZE_IN_PAGES - 1][L1PAGE_TABLE_ENTRY_COUNT];
        };
    };
} packed4_struct;

struct l2table {
    union {
        u8 bytes[PAGE_SIZE];
        struct page_frame page;

        struct page_table_entry entries[L2PAGE_TABLE_ENTRY_COUNT];
    };
} packed4_struct;


/*
 * MMUPR
 */
#define COMPOSE_PROTECT_TYPE(k, r, w, x) \
            (((k) << 3) | ((r) << 2) | ((w) << 1) | (x))

#define DECOMPOSE_DMMU_PROTECT_TYPE(scheme, kr, kw, ur, uw) \
            do {                                            \
                kr = ((scheme) >> 0) & 0x1;                 \
                kw = ((scheme) >> 1) & 0x1;                 \
                ur = ((scheme) >> 2) & 0x1;                 \
                uw = ((scheme) >> 3) & 0x1;                 \
            } while (0)

#define DECOMPOSE_IMMU_PROTECT_TYPE(scheme, kx, ux)         \
            do {                                            \
                kx = ((scheme) >> 0) & 0x1;                 \
                ux = ((scheme) >> 1) & 0x1;                 \
            } while (0)

struct mmu_protect_reg_setup {
    u8 type;
    u8 field;
    u8 immupr;
    u8 dmmupr;
};

#define MMU_PROTECT_SCHEME_TO_REG_IDX { \
    /* 0x0: ____ */ 0,                  \
    /* 0x1: ___X */ 6,                  \
    /* 0x2: ____ */ 0,                  \
    /* 0x3: ____ */ 0,                  \
    /* 0x4: _R__ */ 5,                  \
    /* 0x5: ____ */ 0,                  \
    /* 0x6: _RW_ */ 4,                  \
    /* 0x7: _RWX */ 3,                  \
    /* 0x8: ____ */ 0,                  \
    /* 0x9: ____ */ 0,                  \
    /* 0xa: ____ */ 0,                  \
    /* 0xb: ____ */ 0,                  \
    /* 0xc: ____ */ 0,                  \
    /* 0xd: ____ */ 0,                  \
    /* 0xe: KRW_ */ 2,                  \
    /* 0xf: KRWX */ 1,                  \
}

#define MMU_PROTECT_REG_SETUP { \
    { .type = 0x0 /* ____ */, .field = 0, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0x0 /* K__ U__ */ }, \
    { .type = 0xf /* KRWX */, .field = 1, .immupr = 0x1 /* KX U_ */, .dmmupr = 0x3 /* KRW U__ */ }, \
    { .type = 0xe /* KRW_ */, .field = 2, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0x3 /* KRW U__ */ }, \
    { .type = 0x7 /* _RWX */, .field = 3, .immupr = 0x2 /* K_ UX */, .dmmupr = 0xf /* KRW URW */ }, \
    { .type = 0x6 /* _RW_ */, .field = 4, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0xf /* KRW URW */ }, \
    { .type = 0x4 /* _R__ */, .field = 5, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0x7 /* KRW UR_ */ }, \
    { .type = 0x1 /* ___X */, .field = 6, .immupr = 0x2 /* K_ UX */, .dmmupr = 0x3 /* KRW U__ */ }, \
}


#endif
