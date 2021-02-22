#ifndef __ARCH_MIPS_COMMON_INCLUDE_PRIMS__
#define __ARCH_MIPS_COMMON_INCLUDE_PRIMS__


#include "common/include/inttypes.h"
#include "common/include/abi.h"


/*
 * MIPS barriers
 */
static inline void __atomic_empty()
{
    __asm__ __volatile__ ( "" : : : "memory" );
}

static inline void __atomic_synci()
{
    __asm__ __volatile__ ( "synci;" : : : "memory" );
}

static inline void __atomic_sync()
{
    __asm__ __volatile__ ( "sync;" : : : "memory" );
}


/*
 * Pause
 */
static inline void atomic_pause()
{
    __atomic_empty();
}

static inline void atomic_notify()
{
    __atomic_empty();
}


/*
 * Pipeline barrier
 */
// Execution barrier: Insturctions after atomic_eb execute after DSB completes
static inline void atomic_eb()
{
    __atomic_synci();
}

// Instruction barrier: Instructions after atomic_ib are fetched after atomic_ib completes
static inline void atomic_ib()
{
    __atomic_synci();
}


/*
 * Memory barrier
 */
static inline void atomic_mb()
{
    __atomic_sync();
}

static inline void atomic_rb()
{
    __atomic_sync();
}

static inline void atomic_wb()
{
    __atomic_sync();
}


/*
 * Compare and swap
 */
#if (ARCH_WIDTH == 32)
#define LL  "ll"
#define SC  "sc"
#elif (ARCH_WIDTH == 64)
#define LL  "lld"
#define SC  "scd"
#else
#error "Unsupported arch width"
#endif

static inline ulong atomic_cas_val(volatile void *addr,
                                   ulong old_val, ulong new_val)
{
    ulong tmp, read;

    __asm__ __volatile__ (
        "   .set noreorder;"
        "1: sync;"
        " " LL " %[read], 0(%[ptr]);"
        "   bne  %[read], %[val_old], 2f;"
        "   nop;"
        "   move %[tmp], %[val_new];"
        " " SC " %[tmp], 0(%[ptr]);"
        "   beq  %[tmp], %[zero], 1b;"
        "   nop;"
        "2: sync;"
        "   .set reorder;"
        : [tmp] "=&r" (tmp), [read] "=&r" (read)
        : [ptr] "r" (addr), [zero] "i" (0),
            [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "memory"
    );

    return read;
}


#endif
