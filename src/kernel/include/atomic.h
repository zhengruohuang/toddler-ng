#ifndef __KERNEL_INCLUDE_ATOMIC__
#define __KERNEL_INCLUDE_ATOMIC__


#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/refcount.h"


/*
 * Non-premptive
 */
#define no_prempt() \
    for (int __term = 0, __int_ena = hal_disable_local_int(); !__term; \
            __term = 1, hal_restore_local_int(__int_ena)) \
        for ( ; !__term; __term = 1)


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

#define SPINLOCK_INIT { .value = 0 }
#define SPINLOCK_INIT_LOCKED { .value = 1 }

#define spinlock_exclusive(l) \
    for (spinlock_t *__lock = l; __lock; __lock = NULL) \
        for (spinlock_lock(__lock); __lock; spinlock_unlock(__lock), __lock = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define spinlock_exclusive_int(l) \
    for (spinlock_t *__lock = l; __lock; __lock = NULL) \
        for (spinlock_lock_int(__lock); __lock; spinlock_unlock_int(__lock), __lock = NULL) \
            for (int __term = 0; !__term; __term = 1)

static inline int spinlock_is_locked(spinlock_t *lock)
{
    return lock->locked ? 1 : 0;
}

extern void spinlock_init(spinlock_t *lock);
extern void spinlock_init_locked(spinlock_t *lock);

extern void spinlock_lock(spinlock_t *lock);
extern int spinlock_trylock(spinlock_t *lock);
extern void spinlock_unlock(spinlock_t *lock);

extern void spinlock_lock_int(spinlock_t *lock);
extern int spinlock_trylock_int(spinlock_t *lock);
extern void spinlock_unlock_int(spinlock_t *lock);


/*
 * Spin-based readers-writer lock
 */
typedef struct {
    union {
        volatile ulong value;
        struct {
            volatile ulong write        : 1;
            volatile ulong int_enabled  : 1;
            volatile ulong reads        : sizeof(ulong) * 8 - 2;
        };
    };
} rwlock_t;

#define RWLOCK_INIT { .value = 0 }

extern void rwlock_init(rwlock_t *lock);

extern void rwlock_read_lock(rwlock_t *lock);
extern void rwlock_read_unlock(rwlock_t *lock);
extern void rwlock_write_lock(rwlock_t *lock);
extern void rwlock_write_unlock(rwlock_t *lock);

extern int rwlock_read_lock_int(rwlock_t *lock);
extern void rwlock_read_unlock_int(rwlock_t *lock, int enabled);
extern void rwlock_write_lock_int(rwlock_t *lock);
extern void rwlock_write_unlock_int(rwlock_t *lock);


/*
 * Local-copy based barrier
 */
typedef struct {
    volatile ulong count;
    volatile ulong release;
} local_barrier_t;

typedef struct {
    volatile local_barrier_t *local;
    ulong total;
} barrier_t;

#define BARRIER_INIT(tot) { .local = NULL, .total = tot }

extern void barrier_init(barrier_t *barrier, ulong total);
void barrier_destroy(barrier_t *barrier);

void barrier_wait(barrier_t *barrier);
void barrier_wait_int(barrier_t *barrier);


#endif
