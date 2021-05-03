#ifndef __ARCH_AARCH64_COMMON_INCLUDE_PRIMS_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_PRIMS_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * AArch64 barriers
 */
static inline void atomic_dmb()
{
    // Memory barrier
    // All memory accesses before DMB finishes before memory accesses after DMB
    __asm__ __volatile__ ( "dmb sy;" : : : "memory" );
}

static inline void atomic_dsb()
{
    // Special data barrier
    // Insturctions after DSB execute after DSB completes
    __asm__ __volatile__ ( "dsb sy;" : : : "memory" );
}

static inline void atomic_isb()
{
    // Pipeline flush
    // Instructions after ISB are fetched after ISB completes
    __asm__ __volatile__ ( "isb;" : : : "memory" );
}


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
    atomic_dsb();
}

// Instruction barrier: Instructions after atomic_ib are fetched after atomic_ib completes
static inline void atomic_ib()
{
    atomic_isb();
}


/*
 * Memory barrier
 */
// Full memory barrier
static inline void atomic_mb()
{
    atomic_dmb();
}

// Read barrier
static inline void atomic_rb()
{
    atomic_dmb();
}

// Write barrier
static inline void atomic_wb()
{
    atomic_dmb();
}


/*
 * Compare and swap
 */
static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
//     ulong read = *addr;
//     if (read == old_val) {
//         *addr = new_val;
//     }
//     return read;

    unsigned long read;
    u32 failed = 0;

    __asm__ __volatile__ (
        "1: ldaxr    %[read], [%[ptr]];"
        "   cmp      %[read], %[val_old];"
        "   b.ne     2f;"
        "   stlxr    %w[failed], %[val_new], [%[ptr]];"
        "   cbnz     %w[failed], 1b;"
        "2:;"
        : [failed] "=&r" (failed), [read] "=&r" (read)
        : [ptr] "r" (addr), [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "cc", "memory"
    );

    return read;
}


#endif
