#ifndef __LIBC_INCLUDE_KTH_H__
#define __LIBC_INCLUDE_KTH_H__


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
// TODO: remove
struct __rwlock_flags {
    union {
        unsigned long value;

        struct {
            unsigned long writer        : 1;
            unsigned long num_readers   : sizeof(unsigned long) - 1;
        };
    };
};

typedef struct {
    futex_t rd_futex;
    futex_t wr_futex;
    futex_t wait_futex;
    volatile struct __rwlock_flags flags;
    int max_spin;
} rwlock_t;

#define RWLOCK_MAX_SPIN -1
#define RWLOCK_INITIALIZER { \
    .rd_futex = FUTEX_INITIALIZER, \
    .wr_futex = FUTEX_INITIALIZER, \
    .wait_futex = FUTEX_INITIALIZER, \
    .max_spin = RWLOCK_MAX_SPIN \
}

#define rwlock_exclusive_r(l) \
    for (rwlock_t *__m = l; __m; __m = NULL) \
        for (rwlock_rlock(__m); __m; rwlock_runlock(__m), __m = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define rwlock_exclusive_w(l) \
    for (rwlock_t *__m = l; __m; __m = NULL) \
        for (rwlock_wlock(__m); __m; rwlock_wunlock(__m), __m = NULL) \
            for (int __term = 0; !__term; __term = 1)

extern void rwlock_init(rwlock_t *lock);

extern void rwlock_rlock(rwlock_t *lock);
extern int rwlock_rtrylock(rwlock_t *lock);
extern void rwlock_runlock(rwlock_t *lock);

extern void rwlock_wlock(rwlock_t *lock);
extern int rwlock_wtrylock(rwlock_t *lock);
extern void rwlock_wunlock(rwlock_t *lock);

extern void rwlock_upgrade(rwlock_t *lock);
extern int rwlock_tryupgrade(rwlock_t *lock);
extern void rwlock_downgrade(rwlock_t *lock);


/*
 * Semaphore
 */
typedef struct {
    futex_t futex;
    int max_spin;
} sema_t;

#define SEMA_MAX_SPIN 100
#define SEMA_INITIALIZER { .futex = FUTEX_INITIALIZER, .max_spin = SEMA_MAX_SPIN }

extern void sema_init(sema_t *sema);
extern void sema_init_spin(sema_t *sema, int max_spin);
extern void sema_destroy(sema_t *sema);

extern int sema_wait(sema_t *sema);
extern int sema_wait_spin(sema_t *sema, int max_spin);
extern int sema_trywait(sema_t *sema);

extern int sema_post(sema_t *sema);


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
 * Conditional variable
 */
typedef struct {
    futex_t futex;
    int max_spin;
} cond_t;

#define COND_MAX_SPIN 100
#define COND_INITIALIZER { .futex = FUTEX_INITIALIZER, .max_spin = COND_MAX_SPIN }

static inline int cond_is_signaled(cond_t *cond)
{
    return cond->futex.locked ? 1 : 0;
}

extern void cond_init(cond_t *cond);
extern void cond_init_spin(cond_t *cond, int max_spin);
extern void cond_destroy(cond_t *cond);

extern int cond_wait(cond_t *cond);
extern int cond_wait_spin(cond_t *cond, int max_spin);
extern int cond_wait2(cond_t *cond, mutex_t *mutex);
extern int cond_wait2_spin(cond_t *cond, mutex_t *mutex, int max_spin);

extern int cond_signal(cond_t *cond);


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
extern void kth_yield();


#endif
