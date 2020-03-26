#ifndef __KERNEL_INCLUDE_ATOMIC__
#define __KERNEL_INCLUDE_ATOMIC__


#include "common/include/inttypes.h"
#include "common/include/atomic.h"


/*
 * Generic atomic types
 */
typedef struct {
    volatile ulong value;
} atomic_t;


/*
 * Spin lock
 */
typedef struct {
    union {
        volatile ulong value;
        struct {
            volatile ulong locked       : 1;
            volatile ulong int_enabled  : 1;
        };
    };
} spinlock_t;


extern void spinlock_init(spinlock_t *lock);

extern void spinlock_lock(spinlock_t *lock);
extern void spinlock_unlock(spinlock_t *lock);

extern void spinlock_lock_int(spinlock_t *lock);
extern void spinlock_unlock_int(spinlock_t *lock);


/*
 * Spin-based readers-writer lock
 */
typedef struct {
    union {
        volatile ulong value;
        struct {
            volatile ulong locked       : 1;
            volatile ulong int_enabled  : 1;
            volatile ulong counter      : sizeof(ulong) * 8 - 2;
        };
    };
} rwlock_t;

extern void rwlock_init(rwlock_t *lock);

extern void rwlock_read_lock(rwlock_t *lock);
extern void rwlock_read_unlock(rwlock_t *lock);
extern void rwlock_write_lock(rwlock_t *lock);
extern void rwlock_write_unlock(rwlock_t *lock);

extern void rwlock_read_lock_int(rwlock_t *lock);
extern void rwlock_read_unlock_int(rwlock_t *lock);
extern void rwlock_write_lock_int(rwlock_t *lock);
extern void rwlock_write_unlock_int(rwlock_t *lock);


#endif
