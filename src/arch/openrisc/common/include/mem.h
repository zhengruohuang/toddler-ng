#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_MEM_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_MEM_H__


#include "common/include/inttypes.h"


#define PAGE_SIZE           8192
#define PAGE_BITS           13

#define MAX_NUM_VADDR_BITS  32
#define MAX_NUM_PADDR_BITS  32

typedef u32                 paddr_t;
typedef paddr_t             ppfn_t;
typedef paddr_t             psize_t;


#endif

