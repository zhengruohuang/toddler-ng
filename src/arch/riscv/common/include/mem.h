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


#if (defined(ARCH_RISCV64))
    #define MAX_NUM_VADDR_BITS  39  // Must be 39 or 48
    #define MAX_NUM_PADDR_BITS  64

    #define USER_VADDR_BASE     0x100100000ul   // 4GB + 1MB
    #define USER_VADDR_LIMIT    0x1000000000ul  // 64GB

#elif (defined(ARCH_RISCV32))
    #define MAX_NUM_VADDR_BITS  32
    #define MAX_NUM_PADDR_BITS  34

    #define USER_VADDR_BASE     0x100000ul      // 1MB
    #define USER_VADDR_LIMIT    0xf0000000ul    // 4GB - 256MB

#else
    #error "Unsupported arch width!"

#endif


#endif
