#ifndef __ARCH_RISCV_COMMON_INCLUDE_SYSENTER_H__
#define __ARCH_RISCV_COMMON_INCLUDE_SYSENTER_H__


#include "common/include/inttypes.h"


static inline int sysenter(ulong num, ulong p1, ulong p2, ulong p3,
                           ulong *r1, ulong *r2)
{
    register ulong o0 __asm__ ("x10");
    register ulong o1 __asm__ ("x11");
    register ulong o2 __asm__ ("x12");

    register ulong i0 __asm__ ("x10") = num;
    register ulong i1 __asm__ ("x11") = p1;
    register ulong i2 __asm__ ("x12") = p2;
    register ulong i3 __asm__ ("x13") = p3;
    register ulong i4 __asm__ ("x14") = 0;

    // Currently S-mode ecall traps into SBI, resulting in ecall from S-mode
    // being invisible to HAL
    // As for system calls, instead we use two illegal instructions (two 16-bit
    // zeros). This allows a S-mode kernel thread to trap to HAL, which runs in
    // S-mode, too. The same mechanism applies to user threads as well.
    // The following ecall (which will be skipped by int handler), serves as a
    // magic number to identify this hack
    // Since two 16-bit zeros and an ecall instruction make 8 bytes. The
    // sequence is then aligned to 8-byte boundary to avoid any page fault
    // during int handler
    // Note that two illegal instrs make a 32-bit slot, to avoid any alignment
    // fault on systems without "C" extension
    __asm__ __volatile__ (
        ".balign 8;"
        ".byte 0; .byte 0;"
        ".byte 0; .byte 0;"
        "ecall;"
        : "=r" (o0), "=r" (o1), "=r" (o2)
        : "r" (i0), "r" (i1), "r" (i2), "r" (i3), "r" (i4)
        : "memory"
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
