#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_PRIMS_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_PRIMS_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * OpenRISC barriers
 */
// Execution of context synchronization instruction results in
// completion of all operations inside the processor and a flush of the
// instruction pipelines. When all operations are complete, the RISC core
// resumes with an empty instruction pipeline and fresh context in all units
// (MMU for example).
static inline void atomic_or1k_csync()
{
    __asm__ __volatile__ ( "l.csync" : : : "memory" );
}

// Execution of pipeline synchronization instruction results in
// completion of all instructions that were fetched before l.psync instruction.
// Once all instructions are completed, instructions fetched after l.psync are
// flushed from the pipeline and fetched again.
static inline void atomic_or1k_psync()
{
    __asm__ __volatile__ ( "l.psync" : : : "memory" );
}

// Execution of the memory synchronization instruction results in
// completion of all load/store operations before the RISC core continues.
static inline void atomic_or1k_msync()
{
    __asm__ __volatile__ ( "l.msync" : : : "memory" );
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
    atomic_or1k_psync();
}

// Instruction barrier: Instructions after atomic_ib are fetched after atomic_ib completes
static inline void atomic_ib()
{
    atomic_or1k_psync();
}


/*
 * Memory barrier
 */
// Full memory barrier
static inline void atomic_mb()
{
    atomic_or1k_msync();
}

// Read barrier
static inline void atomic_rb()
{
    atomic_or1k_msync();
}

// Write barrier
static inline void atomic_wb()
{
    atomic_or1k_msync();
}


/*
 * Compare and swap
 */
static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
    ulong read = *addr;
    if (read == old_val) {
        *addr = new_val;
    }
    return read;
}


#endif
