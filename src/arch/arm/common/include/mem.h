#ifndef __ARCH_ARM_COMMON_INCLUDE_MEM_H__
#define __ARCH_ARM_COMMON_INCLUDE_MEM_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


#define PAGE_SIZE           4096
#define PAGE_BITS           12

#define MAX_NUM_VADDR_BITS  32
#define MAX_NUM_PADDR_BITS  32

typedef u32                 paddr_t;
typedef paddr_t             ppfn_t;
typedef paddr_t             psize_t;


#endif
