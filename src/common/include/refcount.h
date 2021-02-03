#ifndef __COMMON_INCLUDE_REFCOUNT_H__
#define __COMMON_INCLUDE_REFCOUNT_H__


#include "common/include/inttypes.h"
#include "common/include/atomic.h"


typedef atomic_t ref_count_t;

#define REF_COUNT_INIT(c) { .value = (c) }


static inline void ref_count_init(ref_count_t *rc, ulong val)
{
    rc->value = val;
    atomic_mb();
}

static inline ulong ref_count_read(ref_count_t *rc)
{
    atomic_mb();
    return rc->value;
}

static inline int ref_count_is_zero(ref_count_t *rc)
{
    atomic_mb();
    return rc->value ? 0 : 1;
}

static inline void ref_count_inc(ref_count_t *rc)
{
    atomic_inc(&rc->value);
    atomic_mb();
}

static inline void ref_count_dec(ref_count_t *rc)
{
    atomic_dec(&rc->value);
    atomic_mb();
}

static inline ulong ref_count_add(ref_count_t *rc, ulong num)
{
    ulong old = atomic_fetch_and_add(&rc->value, num);
    atomic_mb();
    return old;
}

static inline ulong ref_count_sub(ref_count_t *rc, ulong num)
{
    ulong old = atomic_fetch_and_sub(&rc->value, num);
    atomic_mb();
    return old;
}


#endif
