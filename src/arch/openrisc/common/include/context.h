#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


/*
 * Context
 */
struct reg_context {
    union {
        ulong regs[32];

        struct {
            ulong zero, sp, fp;
            ulong a0, a1, a2, a3, a4, a5;
            ulong lr;
            ulong tls;
            ulong v0, v1;
            ulong t0, s0, t1, s1, t2, s2, t3, s3, t4, s4, t5, s5, t6, s6, t7, s7, t8, s8, t9;
        };
    };

    ulong pc;
    ulong sr;
    int delay_slot;
} natural_struct;


#endif
