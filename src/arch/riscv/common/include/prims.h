#ifndef __ARCH_RISCV_COMMON_INCLUDE_PRIMS_H__
#define __ARCH_RISCV_COMMON_INCLUDE_PRIMS_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * RISC-V barriers
 */
static inline void __atomic_fence_i()
{
    __asm__ __volatile__ ( "fence.i" : : : "memory" );
}

static inline void __atomic_fence_r_r()
{
    __asm__ __volatile__ ( "fence r, r" : : : "memory" );
}

static inline void __atomic_fence_w_w()
{
    __asm__ __volatile__ ( "fence w, w" : : : "memory" );
}

static inline void __atomic_fence_rw_rw()
{
    __asm__ __volatile__ ( "fence rw, rw" : : : "memory" );
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
    __atomic_fence_i();
}

// Instruction barrier: Instructions after atomic_ib are fetched after atomic_ib completes
static inline void atomic_ib()
{
    __atomic_fence_i();
}


/*
 * Memory barrier
 */
// Full memory barrier
static inline void atomic_mb()
{
    __atomic_fence_rw_rw();
}

// Read barrier
static inline void atomic_rb()
{
    __atomic_fence_r_r();
}

// Write barrier
static inline void atomic_wb()
{
    __atomic_fence_w_w();
}


/*
 * Compare and swap
 */
#if (defined(ARCH_RISCV32))
#define LR  "lr.w"
#define SC  "sc.w"
#elif (defined(ARCH_RISCV64))
#define LR  "lr.d"
#define SC  "sc.d"
#else
#error "Unsupported arch width"
#endif

static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
//     ulong read = *addr;
//     if (read == old_val) {
//         *addr = new_val;
//     }
//     return read;

    ulong fail, read;

    __asm__ __volatile__ (
        "1: fence rw, rw;"
        " " LR " %[read], (%[ptr]);"
        "   bne  %[read], %[val_old], 2f;"
        " " SC " %[fail], %[val_new], (%[ptr]);"
        "   bnez %[fail], 1b;"
        "2: fence rw, rw;"
        : [fail] "=r" (fail), [read] "=&r" (read)
        : [ptr] "r" (addr), [val_old] "r" (old_val), [val_new] "r" (new_val)
        : "memory"
    );

    return read;
}


#endif
