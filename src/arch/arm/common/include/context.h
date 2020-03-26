#ifndef __ARCH_ARM_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_ARM_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


/*
 * Context
 */
struct reg_context {
    u32 pc;
    u32 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
    union { u32 r13; u32 sp; };
    union { u32 r14; u32 lr; };
    u32 cpsr;
} packedstruct;


#endif
