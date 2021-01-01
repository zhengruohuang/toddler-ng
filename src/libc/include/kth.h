#ifndef __LIBC_INCLUDE_THREADS_H__
#define __LIBC_INCLUDE_THREADS_H__


#include <atomic.h>
#include "common/include/proc.h"


/*
 * TLS
 */
#ifndef decl_tls
#define decl_tls(type, name)    ulong __##name##_tls_offset = -1ul
#endif

#ifndef extern_tls
#define extern_tls(type, name)  extern ulong __##name##_tls_offset
#endif

#ifndef get_tls
#define get_tls(type, name)     ((type *)access_tls_var(&__##name##_tls_offset, sizeof(type)))
#endif

extern void *access_tls_var(ulong *offset, size_t size);


/*
 * Mutex
 */
typedef struct {
    futex_t futex;
    int max_spin;
} mutex_t;

#define MUTEX_MAX_SPIN 100
#define MUTEX_INITIALIZER { .futex = FUTEX_INITIALIZER, .max_spin = MUTEX_MAX_SPIN }

#define mutex_exclusive(l) \
    for (mutex_t *__m = l; __m; __m = NULL) \
        for (mutex_lock(__m); __m; mutex_unlock(__m), __m = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define mutex_exclusive_spin(l, s) \
    for (mutex_t *__m = l; __m; __m = NULL) \
        for (mutex_lock_spin(__m, s); __m; mutex_unlock(__m), __m = NULL) \
            for (int __term = 0; !__term; __term = 1)

static inline int mutex_is_locked(mutex_t *lock)
{
    return lock->futex.locked ? 1 : 0;
}

extern void mutex_init(mutex_t *lock);
extern void mutex_init_spin(mutex_t *lock, int spin);
extern void mutex_destroy(mutex_t *lock);

extern void mutex_lock(mutex_t *lock);
extern void mutex_lock_spin(mutex_t *lock, int spin);
extern int mutex_trylock(mutex_t *lock);
extern void mutex_unlock(mutex_t *lock);


/*
 * RW-lock
 */
typedef volatile struct {
    union {
        ulong value;
        struct {
            ulong locked        : 1;
            ulong num_readers   : sizeof(ulong) - 1;
        };
    };
} fast_rwlock_state_t;

typedef volatile struct {
    fast_rwlock_state_t state;
    ulong rd_wait_obj_id;
    ulong wr_wait_obj_id;
} fast_rwlock_t;

#define FAST_RWLOCK_INITIALIZER { .state.value = 0, .rd_wait_obj_id = 0, .wr_wait_obj_id = 0 }

extern void fast_rwlock_init(fast_rwlock_t *lock);
extern void fast_rwlock_destroy(fast_rwlock_t *lock);

extern void fast_rwlock_rdlock(fast_rwlock_t *lock);
extern int fast_rwlock_rdtrylock(fast_rwlock_t *lock);
extern void fast_rwlock_rdunlock(fast_rwlock_t *lock);

extern void fast_rwlock_wrlock(fast_rwlock_t *lock);
extern int fast_rwlock_wrtrylock(fast_rwlock_t *lock);
extern void fast_rwlock_wrunlock(fast_rwlock_t *lock);


/*
 * Barrier
 */
typedef volatile struct {
    ulong count;
    ulong release;
} barrier_local_t;

typedef struct {
    barrier_local_t *local;
    ulong total;
    ulong wait_obj_id;
} barrier_t;


/*
 * Thread control
 */
typedef unsigned long (*kth_entry_t)(unsigned long param);

typedef struct {
    kth_entry_t entry;
    unsigned long param;
    ulong tid;

    volatile int inited;
    volatile int exited;
    volatile unsigned long ret;

    mutex_t joined;
} kth_t;

extern int kth_create(kth_t *kth, kth_entry_t entry, unsigned long param);
extern kth_t *kth_self();
extern int kth_join(kth_t *kth, ulong *ret);
extern void kth_exit(unsigned long retval);


#endif
