#ifndef __ARCH_DUMMY_COMMON_INCLUDE_SYSENTER_H__
#define __ARCH_DUMMY_COMMON_INCLUDE_SYSENTER_H__


#include "common/include/inttypes.h"


static inline int sysenter(ulong num, ulong p1, ulong p2, ulong p3,
                           ulong *r1, ulong *r2)
{
    return -1;
}


#ifdef FAST_GET_TIB
#undef FAST_GET_TIB
#endif

#define FAST_GET_TIB 1

static inline ulong sysenter_get_tib()
{
    return 0;
}


#endif
