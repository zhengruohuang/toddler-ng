#ifndef __ARCH_X86_COMMON_INCLUDE_MEM_H__
#define __ARCH_X86_COMMON_INCLUDE_MEM_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


#define PAGE_SIZE               4096
#define PAGE_BITS               12
#define USE_GENERIC_PAGE_TABLE  1


#if (defined(ARCH_IA32))
    #define MAX_NUM_VADDR_BITS  32
    #define MAX_NUM_PADDR_BITS  32                  // Must be 32

    #define USER_VADDR_BASE     0x100000ul          // 1MB
    #define USER_VADDR_LIMIT    0xf0000000ul        // 4GB - 256MB

#elif (defined(ARCH_AMD64))
    #define MAX_NUM_VADDR_BITS  48
    #define MAX_NUM_PADDR_BITS  52

    #define USER_VADDR_BASE     0x100100000ul       // 4GB + 1MB
    #define USER_VADDR_LIMIT    0x7f0000000000ul
                            //  0x3f00000000ul      // 256GB - 4GB, 39-bit vaddr
                            //  0x7f0000000000ul    // 128TB - 1TB, 48-bit vaddr

#else
    #error "Unsupported arch width!"

#endif


#if (MAX_NUM_PADDR_BITS > 32)
typedef u64                     paddr_t;
#else
typedef u32                     paddr_t;
#endif
typedef paddr_t                 ppfn_t;
typedef paddr_t                 psize_t;


#endif
