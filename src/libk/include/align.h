#ifndef __LIBK_INCLUDE_ALIGN_H__
#define __LIBK_INCLUDE_ALIGN_H__


#include "common/include/inttypes.h"
#include "libk/include/debug.h"
#include "libk/include/math.h"
#include "libk/include/bit.h"


/*
 * Power of 2
 */
static inline ulong align_up_ulong(ulong val, ulong align)
{
    panic_if(popcount(val) != 1, "Align must be power of 2!\n");
    return ALIGN_UP(val, align);
}

static inline ulong align_down_ulong(ulong val, ulong align)
{
    panic_if(popcount(val) != 1, "Align must be power of 2!\n");
    return ALIGN_DOWN(val, align);
}


/*
 * Any
 */
static inline ulong align_up_ulong_any(ulong val, ulong align)
{
    panic_if(!align, "Align cannot be 0!\n");

    ulong q = 0, r = 0;
    div_ulong(val, align, &q, &r);
    if (r) {
        q++;
        val = mul_ulong(q, align);
    }

    return val;
}

static inline ulong align_down_ulong_any(ulong val, ulong align)
{
    panic_if(!align, "Align cannot be 0!\n");

    ulong q = 0, r = 0;
    div_ulong(val, align, &q, &r);
    if (r) {
        val = mul_ulong(q, align);
    }

    return val;
}


#endif
