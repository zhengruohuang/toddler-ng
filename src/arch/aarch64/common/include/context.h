#ifndef __ARCH_AARCH64_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


/*
 * Context
 */
struct reg_context {
    ulong regs[32];
    ulong pc;
    ulong cpsr;
} natural_struct;


#endif
