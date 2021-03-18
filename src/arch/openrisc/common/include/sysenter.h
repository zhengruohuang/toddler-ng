#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_SYSENTER_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_SYSENTER_H__


#include "common/include/inttypes.h"


static inline int sysenter(ulong num, ulong p1, ulong p2, ulong p3,
                           ulong *r1, ulong *r2)
{
    register ulong o0 __asm__ ("r11");
    register ulong o1 __asm__ ("r4");
    register ulong o2 __asm__ ("r5");

    register ulong i0 __asm__ ("r3") = num;
    register ulong i1 __asm__ ("r4") = p1;
    register ulong i2 __asm__ ("r5") = p2;
    register ulong i3 __asm__ ("r6") = p3;
    register ulong i4 __asm__ ("r7") = 0;

    //    a0 -> r3    - num
    //    v0 -> r11   - success
    // a1-a4 -> r4-r7 - param1-4/return1-2
    __asm__ __volatile__ (
        "l.sys 0;"
        "l.nop;"
        : "=r" (o0), "=r" (o1), "=r" (o2)
        : "r" (i0), "r" (i1), "r" (i2), "r" (i3), "r" (i4)
    );

    if (r1) *r1 = o1;
    if (r2) *r2 = o2;
    int success = (int)(uint)o0;

    return success;
}


#ifdef FAST_GET_TIB
#undef FAST_GET_TIB
#endif

#define FAST_GET_TIB 1

static inline ulong sysenter_get_tib()
{
    ulong value = 0;

    __asm__ __volatile__ (
        "l.ori %[reg], r10, 0;"
        : [reg] "=r" (value)
        :
        : "memory"
    );

    return value;
}


#endif
