#ifndef __ARCH_ALPHA_COMMON_INCLUDE_MEM_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_MEM_H__


#include "common/include/inttypes.h"


#define PAGE_SIZE           8192
#define PAGE_BITS           13

#define MAX_NUM_VADDR_BITS  43
#define MAX_NUM_PADDR_BITS  45

typedef u64                 paddr_t;
typedef paddr_t             ppfn_t;
typedef paddr_t             psize_t;


#endif
