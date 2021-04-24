#ifndef __ARCH_X86_COMMON_INCLUDE_SYSENTER_H__
#define __ARCH_X86_COMMON_INCLUDE_SYSENTER_H__


#include "common/include/inttypes.h"


static inline int sysenter(ulong num, ulong p1, ulong p2, ulong p3,
                           ulong *r1, ulong *r2)
{
    register ulong o0 __asm__ ("%esi");
    register ulong o1 __asm__ ("%eax");
    register ulong o2 __asm__ ("%ebx");

    register ulong i0 __asm__ ("%esi") = num;
    register ulong i1 __asm__ ("%eax") = p1;
    register ulong i2 __asm__ ("%ebx") = p2;
    register ulong i3 __asm__ ("%ecx") = p3;
    register ulong i4 __asm__ ("%edx") = 0;

    __asm__ __volatile__ (
        "int $0x90;"
        : "=r" (o0), "=r" (o1), "=r" (o2)
        : "r" (i0), "r" (i1), "r" (i2), "r" (i3), "r" (i4)
        : "memory", "cc"
    );

    if (r1) *r1 = o1;
    if (r2) *r2 = o2;
    int success = (int)(uint)o0;

    return success;
}


#ifdef FAST_GET_TIB
#undef FAST_GET_TIB
#endif

#define FAST_GET_TIB 0

// static inline ulong sysenter_get_tib()
// {
//     return 0;
// }


#endif
