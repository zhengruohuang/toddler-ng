#ifndef __ARCH_RISCV_COMMON_INCLUDE_MEM_H__
#define __ARCH_RISCV_COMMON_INCLUDE_MEM_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


#define PAGE_SIZE               4096
#define PAGE_BITS               12
#define USE_GENERIC_PAGE_TABLE  1

typedef u64                     paddr_t;
typedef paddr_t                 ppfn_t;
typedef paddr_t                 psize_t;


#if (defined(ARCH_RISCV32))
    #define MAX_NUM_VADDR_BITS  32
    #define MAX_NUM_PADDR_BITS  34

    #define USER_VADDR_BASE     0x100000ul          // 1MB
    #define USER_VADDR_LIMIT    0xf0000000ul        // 4GB - 256MB, RV32

#elif (defined(ARCH_RISCV64))
    #define MAX_NUM_VADDR_BITS  39                  // Must be 48 or 39
    #define MAX_NUM_PADDR_BITS  64

    #define USER_VADDR_BASE     0x100100000ul       // 4GB + 1MB
    #define USER_VADDR_LIMIT    0x3f00000000ul
                            //  0x3f00000000ul      // 256GB - 4GB, RV39
                            //  0x7F0000000000ul    // 128TB - 1TB, RV48

#else
    #error "Unsupported arch width!"

#endif


#endif
