#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "libk/include/bit.h"


/*
 * Multiply
 */
u32 mul_u32(u32 a, u32 b)
{
    u32 um = 0;

    for (int i = 0; i < 32; i++) {
        if (b & (0x1u << i)) {
            um += a << i;
        }
    }

    return um;
}

s32 mul_s32(s32 a, s32 b)
{
    int negs = 0;

    u32 ua = a, ub = b;
    if (ua >> 31) {
        ua = ~ua + 1;
        negs++;
    }
    if (ub >> 31) {
        ub = ~ub + 1;
        negs++;
    }

    u32 um = mul_u32(ua, ub);
    if (negs == 1) {
        um = ~um + 1;
    }

    return (s32)um;
}

u64 mul_u64(u64 a, u64 b)
{
    u64 um = 0;

    for (int i = 0; i < 64; i++) {
        if (b & (0x1u << i)) {
            um += a << i;
        }
    }

    return um;
}

s64 mul_s64(s64 a, s64 b)
{
    int negs = 0;

    u64 ua = a, ub = b;
    if (ua >> 63) {
        ua = ~ua + 1;
        negs++;
    }
    if (ub >> 63) {
        ub = ~ub + 1;
        negs++;
    }

    u64 um = mul_u64(ua, ub);
    if (negs == 1) {
        um = ~um + 1;
    }

    return (s64)um;
}

ulong mul_ulong(ulong a, ulong b)
{
#if (ARCH_WIDTH == 64)
    return (ulong)mul_u64((u64)a, (u64)b);
#elif (ARCH_WIDTH == 32)
    return (ulong)mul_u32((u32)a, (u32)b);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}

ulong mul_long(long a, long b)
{
#if (ARCH_WIDTH == 64)
    return (long)mul_s64((s64)a, (s64)b);
#elif (ARCH_WIDTH == 32)
    return (long)mul_s32((s32)a, (s32)b);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}


/*
 * Divide
 */
void div_u32(u32 a, u32 b, u32 *qout, u32 *rout)
{
    u32 q = 0, r = a;

    for (int i = clz32(b); i >= 0; i--) {
        u32 t = b << i;
        if (t <= r) {
            q |= 0x1u << i;
            r -= t;
        }
    }

    if (qout) *qout = q;
    if (rout) *rout = r;
}

void div_s32(s32 a, s32 b, s32 *qout, s32 *rout)
{
    int neg_a = a >> 31;
    int neg_b = b >> 31;

    u32 ua = a, ub = b;
    if (neg_a) ua = ~ua + 1;
    if (neg_b) ub = ~ub + 1;

    u32 uq = 0, ur = 0;
    div_u32(ua, ub, &uq, &ur);

    if (neg_a && neg_b) {
        ur = ~ur + 1;
    } else if (neg_b) {
        uq = ~uq + 1;
    } else if (neg_a && ur) {
        uq = ~(uq + 1) + 1;
        ur = ~ur + 1 + ub;
    }

    if (qout) *qout = (s32)uq;
    if (rout) *rout = (s32)ur;
}

void div_u64(u64 a, u64 b, u64 *qout, u64 *rout)
{
    u64 q = 0, r = a;

    for (int i = clz64(b); i >= 0; i--) {
        u64 t = b << i;
        if (t <= r) {
            q |= 0x1u << i;
            r -= t;
        }
    }

    if (qout) *qout = q;
    if (rout) *rout = r;
}

void div_s64(s64 a, s64 b, s64 *qout, s64 *rout)
{
    int neg_a = a >> 63;
    int neg_b = b >> 63;

    u32 ua = a, ub = b;
    if (neg_a) ua = ~ua + 1;
    if (neg_b) ub = ~ub + 1;

    u32 uq = 0, ur = 0;
    div_u32(ua, ub, &uq, &ur);

    if (neg_a && neg_b) {
        ur = ~ur + 1;
    } else if (neg_b) {
        uq = ~uq + 1;
    } else if (neg_a && ur) {
        uq = ~(uq + 1) + 1;
        ur = ~ur + 1 + ub;
    }

    if (qout) *qout = (s64)uq;
    if (rout) *rout = (s64)ur;
}

void div_ulong(ulong a, ulong b, ulong *qout, ulong *rout)
{
#if (ARCH_WIDTH == 64)
    u64 q = 0, r = 0;
    div_u64((u64)a, (u64)b, &q, &r);
#elif (ARCH_WIDTH == 32)
    u32 q = 0, r = 0;
    div_u32((u32)a, (u32)b, &q, &r);
#else
    ulong q = 0, r = 0;
    #error Unsupported ARCH_WIDTH
#endif

    if (qout) *qout = q;
    if (rout) *rout = r;
}

void div_long(long a, long b, long *qout, long *rout)
{
#if (ARCH_WIDTH == 64)
    s64 q = 0, r = 0;
    div_s64((s64)a, (s64)b, &q, &r);
#elif (ARCH_WIDTH == 32)
    s32 q = 0, r = 0;
    div_s32((s32)a, (s32)b, &q, &r);
#else
    long q = 0, r = 0;
    #error Unsupported ARCH_WIDTH
#endif

    if (qout) *qout = q;
    if (rout) *rout = r;
}


/*
 * Quo
 */
u64 quo_u64(u64 a, u64 b)
{
    u64 q = 0;
    div_u64(a, b, &q, NULL);
    return q;
}

u64 quo_s64(s64 a, s64 b)
{
    s64 q = 0;
    div_s64(a, b, &q, NULL);
    return q;
}

u32 quo_u32(u32 a, u32 b)
{
    u32 q = 0;
    div_u32(a, b, &q, NULL);
    return q;
}

u64 quo_s32(s32 a, s32 b)
{
    s32 q = 0;
    div_s32(a, b, &q, NULL);
    return q;
}

ulong quo_ulong(ulong a, ulong b)
{
#if (ARCH_WIDTH == 64)
    return (ulong)quo_u64((u64)a, (u64)b);
#elif (ARCH_WIDTH == 32)
    return (ulong)quo_u32((u32)a, (u32)b);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}

long quo_long(long a, long b)
{
#if (ARCH_WIDTH == 64)
    return (long)quo_s64((s64)a, (s64)b);
#elif (ARCH_WIDTH == 32)
    return (long)quo_s32((s32)a, (s32)b);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}


/*
 * Rem
 */
u64 rem_u64(u64 a, u64 b)
{
    u64 r = 0;
    div_u64(a, b, NULL, &r);
    return r;
}

u64 rem_s64(s64 a, s64 b)
{
    s64 r = 0;
    div_s64(a, b, NULL, &r);
    return r;
}

u32 rem_u32(u32 a, u32 b)
{
    u32 r = 0;
    div_u32(a, b, NULL, &r);
    return r;
}

u64 rem_s32(s32 a, s32 b)
{
    s32 r = 0;
    div_s32(a, b, NULL, &r);
    return r;
}

ulong rem_ulong(ulong a, ulong b)
{
#if (ARCH_WIDTH == 64)
    return (ulong)rem_u64((u64)a, (u64)b);
#elif (ARCH_WIDTH == 32)
    return (ulong)rem_u32((u32)a, (u32)b);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}

long rem_long(long a, long b)
{
#if (ARCH_WIDTH == 64)
    return (long)rem_s64((s64)a, (s64)b);
#elif (ARCH_WIDTH == 32)
    return (long)rem_s32((s32)a, (s32)b);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}
