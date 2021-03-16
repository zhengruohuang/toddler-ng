#ifndef __ARCH_RISCV_HAL_INCLUDE_SBI_H__
#define __ARCH_RISCV_HAL_INCLUDE_SBI_H__


#include "common/include/inttypes.h"
#include "common/include/abi.h"


struct sbiret {
    long error;
    long value;
};


static inline struct sbiret __sbi_call(ulong eid, ulong fid, ulong a0, ulong a1,
                                       ulong a2, ulong a3, ulong a4, ulong a5)
{
    register ulong o0 __asm__ ("x10");
    register ulong o1 __asm__ ("x11");

    register ulong i0 __asm__ ("x10") = a0;
    register ulong i1 __asm__ ("x11") = a1;
    register ulong i2 __asm__ ("x12") = a2;
    register ulong i3 __asm__ ("x13") = a3;
    register ulong i4 __asm__ ("x14") = a4;
    register ulong i5 __asm__ ("x15") = a5;
    register ulong i6 __asm__ ("x16") = fid;
    register ulong i7 __asm__ ("x17") = eid;

    __asm__ __volatile__ (
        "ecall;"
        : "=r" (o0), "=r" (o1)
        : "r" (i0), "r" (i1), "r" (i2), "r" (i3), "r" (i4), "r" (i5),
          "r" (i6), "r" (i7)
        : "memory"
    );

    struct sbiret r = { .error = (long)o0, .value = (long)o1 };
    return r;
}


#define sbi_call0(eid, fid)             __sbi_call(eid, fid, 0, 0, 0, 0, 0, 0)
#define sbi_call1(eid, fid, a0)         __sbi_call(eid, fid, a0, 0, 0, 0, 0, 0)
#define sbi_call2(eid, fid, a0, a1)     __sbi_call(eid, fid, a0, a1, 0, 0, 0, 0)
#define sbi_call3(eid, fid, a0, a1, a2) __sbi_call(eid, fid, a0, a1, a2, 0, 0, 0)

static inline struct sbiret sbi_set_timer(u64 stime_value)
{
#if (ARCH_WIDTH == 32)
    ulong a0 = stime_value & 0xffffffffull;
    ulong a1 = stime_value >> 32;
#elif (ARCH_WIDTH == 64)
    ulong a0 = stime_value;
    ulong a1 = 0;
#endif
    return sbi_call2(0x54494D45, 0, a0, a1);
}

static inline struct sbiret sbi_hart_start(ulong hartid, ulong start_addr, ulong opaque)
{
    return sbi_call3(0x48534D, 0, hartid, start_addr, opaque);
}


#endif
