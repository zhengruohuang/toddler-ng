#ifndef __ARCH_MIPS_COMMON_INCLUDE_MEM_H__
#define __ARCH_MIPS_COMMON_INCLUDE_MEM_H__


#include "common/include/inttypes.h"


/*
 * General memory management info
 */
#define PAGE_SIZE           4096
#define PAGE_BITS           12

#define MAX_NUM_VADDR_BITS  32
#define MAX_NUM_PADDR_BITS  32

#define USER_VADDR_LIMIT    0x78000000ul

typedef u32                 paddr_t;
typedef paddr_t             ppfn_t;
typedef paddr_t             psize_t;


/*
 * MIPS-specific segment
 */
#define SEG_USER            0x0ul
#define SEG_DIRECT_CACHED   0x80000000ul
#define SEG_DIRECT_UNCACHED 0xa0000000ul
#define SEG_KERNEL          0xc0000000ul

#define SEG_DIRECT_SIZE     0x20000000ul
#define SEG_DIRECT_MASK     0x1ffffffful


#endif
