#ifndef __ARCH_RISCV_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_RISCV_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


/*
 * Context
 */
struct reg_context {
    ulong my_cpu_hart;

    union {
        ulong regs[32];

        struct {
            ulong x0, x1, x2, x3, x4, x5, x6, x7, x8, x9;
            ulong x10, x11, x12, x13, x14, x15, x16, x17, x18, x19;
            ulong x20, x21, x22, x23, x24, x25, x26, x27, x28, x29;
            ulong x30, x31;
        };

        struct {
            ulong zero, ra, sp, gp, tp;
            ulong t0, t1, t2;
            union { ulong fp; ulong s0; };  // Callee saved
            ulong s1;                       // Callee saved
            union { ulong a0; ulong v0; };
            union { ulong a1; ulong v1; };
            ulong a2, a3, a4, a5, a6, a7;
            ulong s2, s3, s4, s5, s6, s7, s8, s9, s10, s11; // Callee saved
            ulong t3, t4, t5, t6;
        };
    };

    ulong pc;
    ulong status;
} natural_struct;


#endif
