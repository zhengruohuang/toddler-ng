#ifndef __ARCH_X86_COMMON_INCLUDE_ADDR_H__
#define __ARCH_X86_COMMON_INCLUDE_ADDR_H__


#include "common/include/inttypes.h"


// IA32
#if (defined(ARCH_IA32))

typedef u64 paddr_t;
typedef u32 vaddr_t;

// AMD64
#elif (defined(ARCH_AMD64))

typedef u64 paddr_t;
typedef u64 vaddr_t;

// Unknown
#else

#error "Unknown arch type"

#endif


#endif
