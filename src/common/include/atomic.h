#ifndef __COMMON_INCLUDE_ATOMIC_H__
#define __COMMON_INCLUDE_ATOMIC_H__


#include "common/include/prims.h"


/*
 * Read and write
 */
#if (defined(__ARCH_ATOMIC_READ) && __ARCH_ATOMIC_READ)
#else
static inline ulong atomic_read(volatile void *addr)
{
    ulong value = 0;

    atomic_mb();
    value = *(volatile ulong *)addr;
    atomic_mb();

    return value;
}
#endif

#if (defined(__ARCH_ATOMIC_WRITE) && __ARCH_ATOMIC_WRITE)
#else
static inline void atomic_write(volatile ulong *addr, ulong value)
{
    atomic_mb();
    *(volatile ulong *)addr = value;
    atomic_mb();
}
#endif


/*
 * Compare and swap
 */
static inline int atomic_cas_bool(volatile ulong *addr,
                                  ulong old_val, ulong new_val)
{
    ulong read_val = atomic_cas_val(addr, old_val, new_val);
    return read_val == old_val;
}


/*
 * Fetch and add/sub
 * Add/sub and fetch
 */
static inline ulong atomic_fetch_and_add(volatile ulong *addr, ulong add)
{
    ulong old_val, new_val, ret_val;

    do {
        old_val = *addr;
        new_val = old_val + add;
        ret_val = atomic_cas_val(addr, old_val, new_val);
    } while (ret_val != old_val);

    return ret_val;
}

static inline ulong atomic_fetch_and_sub(volatile ulong *addr, ulong sub)
{
    ulong old_val, new_val, ret_val;

    do {
        old_val = *addr;
        new_val = old_val - sub;
        ret_val = atomic_cas_val(addr, old_val, new_val);
    } while (ret_val != old_val);

    return ret_val;
}

static inline ulong atomic_add_and_fetch(volatile ulong *addr, ulong add)
{
    ulong old_val, new_val, ret_val;

    do {
        old_val = *addr;
        new_val = old_val + add;
        ret_val = atomic_cas_val(addr, old_val, new_val);
    } while (ret_val != old_val);

    return new_val;
}

static inline ulong atomic_sub_and_fetch(volatile ulong *addr, ulong sub)
{
    ulong old_val, new_val, ret_val;

    do {
        old_val = *addr;
        new_val = old_val - sub;
        ret_val = atomic_cas_val(addr, old_val, new_val);
    } while (ret_val != old_val);

    return new_val;
}


/*
 * Inc/Dec
 */
static inline void atomic_inc(volatile ulong *addr)
{
    atomic_fetch_and_add(addr, 1);
}

static inline void atomic_dec(volatile ulong *addr)
{
    atomic_fetch_and_sub(addr, 1);
}


#endif

