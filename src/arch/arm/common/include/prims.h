#ifndef __ARCH_ARM_COMMON_INCLUDE_PRIMS_H__
#define __ARCH_ARM_COMMON_INCLUDE_PRIMS_H__


#include "common/include/inttypes.h"


/*
 * ARM barriers
 */
// Wait for event
static inline void __atomic_wfe()
{
    __asm__ __volatile__ ( "wfe" : : : "memory" );
}

// Send event
static inline void __atomic_sev()
{
    __asm__ __volatile__ ( "sev" : : : "memory" );
}

// Special data barrier
// Insturctions after DSB execute after DSB completes
static inline void __atomic_dsb()
{
    __asm__ __volatile__ ( "dsb;" : : : "memory" );
}

// Pipeline flush
// Instructions after ISB are fetched after ISB completes
static inline void __atomic_isb()
{
    __asm__ __volatile__ ( "isb;" : : : "memory" );
}

// Full memory barrier
static inline void __atomic_dmb()
{
    __asm__ __volatile__ ( "dmb;" : : : "memory" );
}


/*
 * Pause
 */
static inline void atomic_pause()
{
    __atomic_wfe();
}

static inline void atomic_notify()
{
    __atomic_sev();
}


/*
 * Pipeline barrier
 */
// Execution barrier: Insturctions after atomic_eb execute after DSB completes
static inline void atomic_eb()
{
    __atomic_dsb();
}

// Instruction barrier: Instructions after atomic_ib are fetched after atomic_ib completes
static inline void atomic_ib()
{
    __atomic_isb();
}


/*
 * Memory barrier
 */
// Full memory barrier
static inline void atomic_mb()
{
    __atomic_dmb();
}

// Read barrier
static inline void atomic_rb()
{
    __atomic_dmb();
}

// Write barrier
static inline void atomic_wb()
{
    __atomic_dmb();
}


/*
 * Compare and swap
 */
static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
    ulong fail, read;

    __asm__ __volatile__ (
        "dmb;"
        "1: ldrex %[read], [%[ptr], #0];"
        "   cmp %[read], %[val_old];"
        "   bne 2f;"
        "   strex %[fail], %[val_new], [%[ptr], #0];"
        "   cmp %[fail], #1;"
        "   beq 1b;"
        "2:;"
        "dmb;"
        : [fail] "=&r" (fail), [read] "=&r" (read)
        : [ptr] "r" (addr), [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "cc", "memory"
    );

    return read;
}


#endif
