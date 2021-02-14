#ifndef __ARCH_MIPS_COMMON_INCLUDE_SYSENTER_H__
#define __ARCH_MIPS_COMMON_INCLUDE_SYSENTER_H__


#include "common/include/inttypes.h"
// #include "common/include/msr.h"

static inline int sysenter(ulong num, ulong p1, ulong p2, ulong p3,
                           ulong *r1, ulong *r2)
{
    register ulong o0 __asm__ ("$2");
    register ulong o1 __asm__ ("$4");
    register ulong o2 __asm__ ("$5");

    register ulong i0 __asm__ ("$2") = num;
    register ulong i1 __asm__ ("$4") = p1;
    register ulong i2 __asm__ ("$5") = p2;
    register ulong i3 __asm__ ("$6") = p3;
    register ulong i4 __asm__ ("$7") = 0;

    //    v0 -> $2    - num/success
    // a0-a3 -> $4-$7 - param1-4/return1-2
    __asm__ __volatile__ (
        "syscall;"
        "nop;"
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

#define FAST_GET_TIB 0

// static inline ulong sysenter_get_tib()
// {
//     ulong tib = 0;
//     read_software_thread_id(tib);
//     return tib;
// }


#endif
