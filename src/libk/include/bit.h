#ifndef __LIBK_INCLUDE_BIT_H__
#define __LIBK_INCLUDE_BIT_H__


#include "common/include/inttypes.h"


/*
 * Alignment
 */
#define ALIGN_UP(s, a)      (((s) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(s, a)    ((s) & ~((a) - 1))
#define ALIGNED(s, a)       (!((s) & ((a) - 1)))


/*
 * Bit
 */
extern u16 swap_endian16(u16 val);
extern u32 swap_endian32(u32 val);
extern u64 swap_endian64(u64 val);
extern ulong swap_endian(ulong val);
extern u16 swap_big_endian16(u16 val);
extern u32 swap_big_endian32(u32 val);
extern u64 swap_big_endian64(u64 val);
extern ulong swap_big_endian(ulong val);
extern u16 swap_little_endian16(u16 val);
extern u32 swap_little_endian32(u32 val);
extern u64 swap_little_endian64(u64 val);
extern ulong swap_little_endian(ulong val);

extern int popcount64(u64 val);
extern int popcount32(u32 val);
extern int popcount16(u16 val);
extern int popcount(ulong val);

extern int clz64(u64 val);
extern int clz32(u32 val);
extern int clz16(u16 val);
extern int clz(ulong val);

extern int ctz64(u64 val);
extern int ctz32(u32 val);
extern int ctz16(u16 val);
extern int ctz(ulong val);

extern u64 align_to_power2_next64(u64 val);
extern u32 align_to_power2_next32(u32 val);
extern ulong align_to_power2_next(ulong val);


#endif
