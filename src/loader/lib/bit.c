#include "common/include/inttypes.h"
#include "common/include/arch.h"


/*
 * Endian
 */
u16 swap_endian16(u16 val)
{
    u16 r = val & 0xff;
    u16 l = (val >> 8) & 0xff;

    u16 swap = (r << 8) | l;
    return swap;
}

u32 swap_endian32(u32 val)
{
    u32 rr = val & 0xff;
    u32 rl = (val >> 8) & 0xff;
    u32 lr = (val >> 16) & 0xff;
    u32 ll = (val >> 24) & 0xff;

    u32 swap = (rr << 24) | (rl << 16) | (lr << 8) | ll;
    return swap;
}

u64 swap_endian64(u64 val)
{
    u64 rrr = val & 0xffull;
    u64 rrl = (val >> 8) & 0xffull;

    u64 rlr = (val >> 16) & 0xffull;
    u64 rll = (val >> 24) & 0xffull;

    u64 lrr = (val >> 32) & 0xffull;
    u64 lrl = (val >> 40) & 0xffull;

    u64 llr = (val >> 48) & 0xffull;
    u64 lll = (val >> 56) & 0xffull;

    u64 swap = (rrr << 56) | (rrl << 48) |
        (rlr << 40) | (rll << 32) |
        (lrr << 24) | (lrl << 16) |
        (llr << 8) | lll;

    return swap;
}

ulong swap_endian(ulong val)
{
#if (ARCH_WIDTH == 64)
    return (ulong)swap_endian64((u64)val);
#elif (ARCH_WIDTH == 32)
    return (ulong)swap_endian32((u32)val);
#elif (ARCH_WIDTH == 16)
    return (ulong)swap_endian16((u16)val);
#else
    #error Unsupported ARCH_WIDTH
#endif
}

u16 swap_big_endian16(u16 val)
{
#if (ARCH_BIG_ENDIAN)
    return val;
#else
    return swap_endian16(val);
#endif
}

u32 swap_big_endian32(u32 val)
{
#if (ARCH_BIG_ENDIAN)
    return val;
#else
    return swap_endian32(val);
#endif
}

u64 swap_big_endian64(u64 val)
{
#if (ARCH_BIG_ENDIAN)
    return val;
#else
    return swap_endian64(val);
#endif
}

ulong swap_big_endian(ulong val)
{
#if (ARCH_BIG_ENDIAN)
    return val;
#else
    return swap_endian(val);
#endif
}

u16 swap_little_endian16(u16 val)
{
#if (ARCH_LITTLE_ENDIAN)
    return val;
#else
    return swap_endian16(val);
#endif
}

u32 swap_little_endian32(u32 val)
{
#if (ARCH_LITTLE_ENDIAN)
    return val;
#else
    return swap_endian32(val);
#endif
}

u64 swap_little_endian64(u64 val)
{
#if (ARCH_LITTLE_ENDIAN)
    return val;
#else
    return swap_endian64(val);
#endif
}

ulong swap_little_endian(ulong val)
{
#if (ARCH_LITTLE_ENDIAN)
    return val;
#else
    return swap_endian(val);
#endif
}


/*
 * Count 1's
 */
int popcount64(u64 val)
{
    int count = 0;

    for (int i = 0; i < 64; i++) {
        if (val & (0x1ull << i)) {
            count++;
        }
    }

    return count;
}

int popcount32(u32 val)
{
    int count = 0;

    for (int i = 0; i < 32; i++) {
        if (val & (0x1 << i)) {
            count++;
        }
    }

    return count;
}

int popcount16(u16 val)
{
    int count = 0;

    for (int i = 0; i < 16; i++) {
        if (val & (0x1 << i)) {
            count++;
        }
    }

    return count;
}

int popcount(ulong val)
{
#if (ARCH_WIDTH == 64)
    return (ulong)popcount64((u64)val);
#elif (ARCH_WIDTH == 32)
    return (ulong)popcount32((u32)val);
#elif (ARCH_WIDTH == 16)
    return (ulong)popcount16((u16)val);
#else
    #error Unsupported ARCH_WIDTH
#endif
}


/*
 * Count leading zero
 */
int clz64(u64 val)
{
    int lead_zeros = 0;

    for (int i = 0; i < 64 && !(val >> 63); i++, val <<= 1) {
        lead_zeros++;
    }

    return lead_zeros;
}

int clz32(u32 val)
{
    int lead_zeros = 0;

    for (int i = 0; i < 32 && !(val >> 31); i++, val <<= 1) {
        lead_zeros++;
    }

    return lead_zeros;
}

int clz16(u16 val)
{
    int lead_zeros = 0;

    for (int i = 0; i < 16 && !(val >> 16); i++, val <<= 1) {
        lead_zeros++;
    }

    return lead_zeros;
}

int clz(ulong val)
{
#if (ARCH_WIDTH == 64)
    return (ulong)clz64((u64)val);
#elif (ARCH_WIDTH == 32)
    return (ulong)clz32((u32)val);
#elif (ARCH_WIDTH == 16)
    return (ulong)clz16((u16)val);
#else
    #error Unsupported ARCH_WIDTH
#endif
}


/*
 * Align to power of 2
 */
u64 align_to_power2_next64(u64 val)
{
    if (!val) {
        return 0;
    }

    int pos = 0;
    for (u64 shift = val; shift; pos++, shift >>= 1);

    u64 aligned = 0x1ull << pos;
    u64 prev = 0x1ull << (pos - 1);
    if (prev == val) {
        return val;
    }

    return aligned;
}

u32 align_to_power2_next32(u32 val)
{
    if (!val) {
        return 0;
    }

    int pos = 0;
    for (u32 shift = val; shift; pos++, shift >>= 1);

    u32 aligned = 0x1ull << pos;
    u32 prev = 0x1ull << (pos - 1);
    if (prev == val) {
        return val;
    }

    return aligned;
}

ulong align_to_power2_next(ulong val)
{
#if (ARCH_WIDTH == 64)
    return (ulong)align_to_power2_next64((u64)val);
#elif (ARCH_WIDTH == 32)
    return (ulong)align_to_power2_next32((u32)val);
#else
    #error Unsupported ARCH_WIDTH
#endif
}
