#ifndef __ARCH_M68K_COMMON_INCLUDE_ATOMIC_H__
#define __ARCH_M68K_COMMON_INCLUDE_ATOMIC_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


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
 * Memory barrier
 */
static inline void atomic_mb()
{
    __asm__ __volatile__ ( "" : : : "memory" );
}

static inline void atomic_rb()
{
    __asm__ __volatile__ ( "" : : : "memory" );
}

static inline void atomic_wb()
{
    __asm__ __volatile__ ( "" : : : "memory" );
}


/*
 * Read and write
 */
static inline ulong atomic_read(volatile void *addr)
{
    ulong value = 0;

    atomic_mb();
    value = *(volatile ulong *)addr;
    atomic_mb();

    return value;
}

static inline void atomic_write(volatile ulong *addr, ulong value)
{
    atomic_mb();
    *(volatile ulong *)addr = value;
    atomic_mb();
}


/*
 * Compare and swap
 */
static inline int atomic_cas_bool(volatile ulong *addr,
                             ulong old_val, ulong new_val)
{
    ulong read = *addr;
    if (read == old_val) {
        *addr = new_val;
    }
    return read == old_val;
}

static inline ulong atomic_cas_val(volatile ulong *addr,
                                   ulong old_val, ulong new_val)
{
    ulong read = *addr;
    if (read == old_val) {
        *addr = new_val;
    }
    return read;
}


/*
 * Fetch and add
 */
static inline void atomic_inc(volatile ulong *addr)
{
    ++*addr;
}

static inline void atomic_dec(volatile ulong *addr)
{
    --*addr;
}


#endif
