#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_TLB_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_TLB_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * MMUPR
 */
struct mmu_protect_reg_setup {
    u8 field;
    u8 type;
    u8 immupr;
    u8 dmmupr;
    u8 kernel, read, write, exec;
};

struct mmu_protect_attri {
    u8 field;
    u8 type;
    u8 k, r, w, x;
};

#define MMU_PROTECT_SCHEME_TO_REG_IDX { \
    /* 4'b0000, 0x0: ____ */ 0,         \
    /* 4'b0001, 0x1: ___X */ 6,         \
    /* 4'b0010, 0x2: ____ */ 0,         \
    /* 4'b0011, 0x3: ____ */ 0,         \
    /* 4'b0100, 0x4: _R__ */ 5,         \
    /* 4'b0101, 0x5: ____ */ 0,         \
    /* 4'b0110, 0x6: _RW_ */ 4,         \
    /* 4'b0111, 0x7: _RWX */ 3,         \
    /* 4'b1000, 0x8: ____ */ 0,         \
    /* 4'b1001, 0x9: ____ */ 0,         \
    /* 4'b1010, 0xa: ____ */ 0,         \
    /* 4'b1011, 0xb: ____ */ 0,         \
    /* 4'b1100, 0xc: ____ */ 0,         \
    /* 4'b1101, 0xd: ____ */ 0,         \
    /* 4'b1110, 0xe: KRW_ */ 2,         \
    /* 4'b1111, 0xf: KRWX */ 1,         \
}

#define MMU_PROTECT_REG_SETUP { \
    { .field = 0, .type = 0x0 /* ____ */, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0x0 /* K__ U__ */ }, \
    { .field = 1, .type = 0xf /* KRWX */, .immupr = 0x1 /* KX U_ */, .dmmupr = 0x3 /* KRW U__ */ }, \
    { .field = 2, .type = 0xe /* KRW_ */, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0x3 /* KRW U__ */ }, \
    { .field = 3, .type = 0x7 /* _RWX */, .immupr = 0x2 /* K_ UX */, .dmmupr = 0xf /* KRW URW */ }, \
    { .field = 4, .type = 0x6 /* _RW_ */, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0xf /* KRW URW */ }, \
    { .field = 5, .type = 0x4 /* _R__ */, .immupr = 0x0 /* K_ U_ */, .dmmupr = 0x7 /* KRW UR_ */ }, \
    { .field = 6, .type = 0x1 /* ___X */, .immupr = 0x2 /* K_ UX */, .dmmupr = 0x3 /* KRW U__ */ }, \
}

#define MMU_PROTECT_ATTRI_SETUP { \
    { .field = 0, .type = 0x0 /* ____ */, .k = 0, .r = 0, .w = 0, .x = 0 }, \
    { .field = 1, .type = 0xf /* KRWX */, .k = 1, .r = 1, .w = 1, .x = 1 }, \
    { .field = 2, .type = 0xe /* KRW_ */, .k = 1, .r = 1, .w = 1, .x = 0 }, \
    { .field = 3, .type = 0x7 /* _RWX */, .k = 0, .r = 1, .w = 1, .x = 1 }, \
    { .field = 4, .type = 0x6 /* _RW_ */, .k = 0, .r = 1, .w = 1, .x = 0 }, \
    { .field = 5, .type = 0x4 /* _R__ */, .k = 0, .r = 1, .w = 0, .x = 0 }, \
    { .field = 6, .type = 0x1 /* ___X */, .k = 0, .r = 0, .w = 0, .x = 1 }, \
}


#define compose_prot_type(k, r, w, x) \
    ((((k) ? 0x1 : 0) << 3) | (((r) ? 0x1 : 0) << 2) | \
     (((w) ? 0x1 : 0) << 1) | (((x) ? 0x1 : 0) << 0))

#define decompose_dmmu_prot_type(dmmupr, kr, kw, ur, uw) \
    do {                                            \
        kr = ((dmmupr) >> 0) & 0x1;                 \
        kw = ((dmmupr) >> 1) & 0x1;                 \
        ur = ((dmmupr) >> 2) & 0x1;                 \
        uw = ((dmmupr) >> 3) & 0x1;                 \
    } while (0)

#define decompose_immu_prot_type(immupr, kx, ux)    \
    do {                                            \
        kx = ((immupr) >> 0) & 0x1;                 \
        ux = ((immupr) >> 1) & 0x1;                 \
    } while (0)


static inline int get_prot_field(int k, int r, int w, int x)
{
    const int protect_scheme_to_reg_idx[] = MMU_PROTECT_SCHEME_TO_REG_IDX;
    const int t = compose_prot_type(k, r, w, x);
    return protect_scheme_to_reg_idx[t];
}

static inline void get_prot_attri(int field, int *k, int *r, int *w, int *x)
{
    const struct mmu_protect_attri protect_attris[] = MMU_PROTECT_ATTRI_SETUP;
    const struct mmu_protect_attri attri = protect_attris[field];

    if (k) *k = attri.k;
    if (r) *r = attri.r;
    if (w) *w = attri.w;
    if (x) *x = attri.x;
}


#endif
