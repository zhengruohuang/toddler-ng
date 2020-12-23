#ifndef __COMMON_INCLUDE_REFCOUNT_H__
#define __COMMON_INCLUDE_REFCOUNT_H__


#include "common/include/inttypes.h"
#include "common/include/atomic.h"


typedef atomic_t ref_count_t;

static inline void ref_count_init(ref_count_t *rc, ulong val)
{
    rc->value = val;
    atomic_mb();
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


#endif
