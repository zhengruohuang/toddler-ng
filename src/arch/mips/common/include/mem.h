#ifndef __ARCH_MIPS_COMMON_INCLUDE_MEM_H__
#define __ARCH_MIPS_COMMON_INCLUDE_MEM_H__

#include "common/include/abi.h"

// MIPS32
#if (ARCH_WIDTH == 32)
#include "common/include/mem32.h"

// MIPS64
#elif (ARCH_WIDTH == 64)
#include "common/include/mem64.h"

#else
#error "Unknown arch width"
#endif

#endif
