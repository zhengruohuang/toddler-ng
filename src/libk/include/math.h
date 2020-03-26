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

extern void div_u32(u32 a, u32 b, u32 *qout, u32 *rout);
extern void div_s32(s32 a, s32 b, s32 *qout, s32 *rout);
extern void div_u64(u64 a, u64 b, u64 *qout, u64 *rout);
extern void div_s64(s64 a, s64 b, s64 *qout, s64 *rout);


#endif
