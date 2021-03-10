#ifndef __ARCH_DUMMY_COMMON_INCLUDE_MEM_H__
#define __ARCH_DUMMY_COMMON_INCLUDE_MEM_H__


#include "common/include/inttypes.h"


#define PAGE_SIZE           4096
#define PAGE_BITS           12

#define MAX_NUM_VADDR_BITS  32
#define MAX_NUM_PADDR_BITS  32

#define USER_VADDR_BASE     0x100000ul
#define USER_VADDR_LIMIT    0xf0000000ul

typedef u32                 paddr_t;
typedef paddr_t             ppfn_t;
typedef paddr_t             psize_t;


#endif
