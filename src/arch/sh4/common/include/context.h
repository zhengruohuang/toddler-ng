#ifndef __ARCH_SH4_COMMON_INCLUDE_CONTEXT_H__
#define __ARCH_SH4_COMMON_INCLUDE_CONTEXT_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


/*
 * Context
 */
struct reg_context {
    ulong regs[32];
} natural_struct;


#endif
