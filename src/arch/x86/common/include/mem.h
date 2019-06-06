#ifndef __ARCH_X86_COMMON_INCLUDE_MEM_H__
#define __ARCH_X86_COMMON_INCLUDE_MEM_H__


#include "common/include/arch.h"


#if (defined(ARCH_IA32))
#include "common/include/mem32.h"

#elif (defined(ARCH_AMD64))
#include "common/include/mem64.h"

#else
#error "Unknown arch!"
#endif


#endif
