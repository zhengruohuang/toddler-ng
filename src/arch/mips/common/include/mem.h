#ifndef __ARCH_MIPS_COMMON_INCLUDE_MEM_H__
#define __ARCH_MIPS_COMMON_INCLUDE_MEM_H__


#include "common/include/abi.h"
#include "common/include/inttypes.h"


/*
 * MIPS32
 */
#if (ARCH_WIDTH == 32)
    /*
     * General memory management info
     */
    #define PAGE_SIZE               4096
    #define PAGE_BITS               12
    #define USE_GENERIC_PAGE_TABLE  1

    #define MAX_NUM_VADDR_BITS      32
    #define MAX_NUM_PADDR_BITS      32

    #define ASID_LIMIT              256
    #define USER_VADDR_BASE         0x100000ul
    #define USER_VADDR_LIMIT        0x78000000ul

    typedef u32                     paddr_t;
    typedef paddr_t                 ppfn_t;
    typedef paddr_t                 psize_t;

    /*
     * MIPS-specific segment
     */
    #define SEG_USER                0x0ul
    #define SEG_DIRECT_CACHED       0x80000000ul
    #define SEG_DIRECT_UNCACHED     0xa0000000ul
    #define SEG_KERNEL              0xc0000000ul
    #define SEG_DIRECT_CACHED_LOW   SEG_DIRECT_CACHED
    #define SEG_DIRECT_UNCACHED_LOW SEG_DIRECT_UNCACHED

    #define SEG_USER_SIZE           0x80000000ul
    #define SEG_DIRECT_SIZE         0x20000000ul
    #define SEG_DIRECT_MASK         0x1ffffffful
    #define SEG_DIRECT_SIZE_LOW     SEG_DIRECT_SIZE
    #define SEG_DIRECT_MASK_LOW     SEG_DIRECT_MASK


/*
 * MIPS64
 */
#elif (ARCH_WIDTH == 64)
    /*
     * General memory management info
     */
    #define PAGE_SIZE               4096
    #define PAGE_BITS               12
    #define USE_GENERIC_PAGE_TABLE  1

    #define MAX_NUM_VADDR_BITS      64
    #define MAX_NUM_PADDR_BITS      48

    #define ASID_LIMIT              256
    #define USER_VADDR_BASE         0x100000ul
    #define USER_VADDR_LIMIT        0xff00000000ul
                                    //0x3f00000000000000ul

    typedef u64                     paddr_t;
    typedef paddr_t                 ppfn_t;
    typedef paddr_t                 psize_t;

    /*
     * MIPS-specific segment
     */
    #define SEG_USER                0x0ul
    #define SEG_DIRECT_CACHED       0x9000000000000000ul
    #define SEG_DIRECT_UNCACHED     0x9800000000000000ul
    #define SEG_KERNEL              0xc000000000000000ul
    #define SEG_DIRECT_CACHED_LOW   0xffffffff80000000ul
    #define SEG_DIRECT_UNCACHED_LOW 0xffffffffa0000000ul

    #define SEG_USER_SIZE           0x4000000000000000ul
    #define SEG_DIRECT_SIZE         0x800000000000000ul
    #define SEG_DIRECT_MASK         0x7fffffffffffffful
    #define SEG_DIRECT_SIZE_LOW     0x20000000ul
    #define SEG_DIRECT_MASK_LOW     0x1ffffffful


/*
 * Unsupported width
 */
#else
    #error "Unsupported arch width"
#endif


#endif
