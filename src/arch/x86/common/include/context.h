#ifndef __ARCH_X86_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_X86_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"
#include "common/include/abi.h"


/*
 * Context
 */
struct reg_context {
    ulong ip, bp, sp, flags;
    ulong cs, ds, es, fs, gs, ss;
    ulong ax, bx, cx, dx, si, di;

#if (ARCH_WIDTH == 64)
    ulong r8, r9, r10, r11, r12, r13, r14, r15;
#endif
} natural_struct;


struct int_reg_context {
    // Seg regs
    ulong kernel_ss;
    ulong gs;
    ulong fs;
    ulong es;
    ulong ds;

    // AMD64 additional GPRs
#if (ARCH_WIDTH == 64)
    ulong r8, r9, r10, r11, r12, r13, r14, r15;
#endif

    // GPR
    ulong di;
    ulong si;
    ulong bp;
    ulong bx;
    ulong dx;
    ulong cx;
    ulong ax;

    // Pushed by interrupt handler
    ulong except, code;

    // Pushed by HW upon interrupt
    ulong ip, cs, flags;
    ulong sp, ss;
} natural_struct;


#endif
