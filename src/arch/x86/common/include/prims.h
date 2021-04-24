#ifndef __ARCH_X86_COMMON_INCLUDE_PRIMS_H__
#define __ARCH_X86_COMMON_INCLUDE_PRIMS_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


/*
 * Pause
 */
static inline void atomic_pause()
{
    __asm__ __volatile__ ( "" : : : "memory" );
}

static inline void atomic_notify()
{
    __asm__ __volatile__ ( "" : : : "memory" );
}


/*
 * Pipeline barrier
 */
// Execution barrier: Insturctions after atomic_eb execute after DSB completes
static inline void atomic_eb()
{
    __asm__ __volatile__ ( "mfence" : : : "memory" );
}

// Instruction barrier: Instructions after atomic_ib are fetched after atomic_ib completes
static inline void atomic_ib()
{
    __asm__ __volatile__ ( "mfence" : : : "memory" );
}


/*
 * Memory barrier
 */
// Full memory barrier
static inline void atomic_mb()
{
    __asm__ __volatile__ ( "mfence" : : : "memory" );
}

// Read barrier
static inline void atomic_rb()
{
    __asm__ __volatile__ ( "lfence" : : : "memory" );
}

// Write barrier
static inline void atomic_wb()
{
    __asm__ __volatile__ ( "sfence" : : : "memory" );
}


/*
 * Compare and swap
 */
static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
    ulong read;

    __asm__ __volatile__ (
#if (ARCH_WIDTH == 32)
        "movl %%edx, %%eax;"
        "lock cmpxchg %%ebx, (%%esi);"
#elif (ARCH_WIDTH == 64)
        "movq %%rdx, %%rax;"
        "lock cmpxchg %%rbx, (%%rsi);"
#endif
        : "=a" (read)
        : "d" (old_val), "b" (new_val), "S" (addr)
        : "memory", "cc"
    );

    return read;
}


#endif
