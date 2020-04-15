#ifndef __LIBK_INCLUDE_MATH_H__
#define __LIBK_INCLUDE_MATH_H__


#include "common/include/inttypes.h"


/*
 * Math
 */
extern u32 mul_u32(u32 a, u32 b);
extern s32 mul_s32(s32 a, s32 b);
extern u64 mul_u64(u64 a, u64 b);
extern s64 mul_s64(s64 a, s64 b);
extern ulong mul_ulong(ulong a, ulong b);
extern ulong mul_long(long a, long b);

extern void div_u32(u32 a, u32 b, u32 *qout, u32 *rout);
extern void div_s32(s32 a, s32 b, s32 *qout, s32 *rout);
extern void div_u64(u64 a, u64 b, u64 *qout, u64 *rout);
extern void div_s64(s64 a, s64 b, s64 *qout, s64 *rout);
extern void div_ulong(ulong a, ulong b, ulong *qout, ulong *rout);
extern void div_long(long a, long b, long *qout, long *rout);

extern u64 quo_u64(u64 a, u64 b);
extern u64 quo_s64(s64 a, s64 b);
extern u32 quo_u32(u32 a, u32 b);
extern u64 quo_s32(s32 a, s32 b);
extern ulong quo_ulong(ulong a, ulong b);
extern long quo_long(long a, long b);

extern u64 rem_u64(u64 a, u64 b);
extern u64 rem_s64(s64 a, s64 b);
extern u32 rem_u32(u32 a, u32 b);
extern u64 rem_s32(s32 a, s32 b);
extern ulong rem_ulong(ulong a, ulong b);
extern long rem_long(long a, long b);


#endif
