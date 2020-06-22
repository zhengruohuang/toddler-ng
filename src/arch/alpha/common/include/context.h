#ifndef __ARCH_ALPHA_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


/*
 * Context
 */
struct reg_context {
    ulong regs[32];
} natural_struct;


struct proc_ctrl_block {
    u64 ksp;
    u64 usp;
    u64 ptbr;
    u32 pcc;
    u32 asn;
    u64 unique;
    union {
        struct {
            u64 fen     : 1;    // FP enabled
            u64 fres1   : 31;
            u64 imb     : 1;    // IMB issued
            u64 fres2   : 29;
            u64 pme     : 1;    // PMU enabled
            u64 fres3   : 1;
        };
        u64 flags;
    };
    u64 compaq[2];
} aligned_struct(64);


#endif
