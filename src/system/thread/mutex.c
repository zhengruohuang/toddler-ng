#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"
#include "libsys/include/futex.h"
#include "libk/include/debug.h"


void mutex_init(mutex_t *lock)
{
    futex_init(&lock->futex);
    lock->max_spin = MUTEX_MAX_SPIN;
}

void mutex_init_spin(mutex_t *lock, int max_spin)
{
    futex_init(&lock->futex);
    lock->max_spin = max_spin;
}

void mutex_destroy(mutex_t *lock)
{
    futex_destory(&lock->futex);
}

void mutex_lock(mutex_t *lock)
{
    futex_lock(&lock->futex, lock->max_spin);
}

int mutex_trylock(mutex_t *lock)
{
    return futex_trylock(&lock->futex);
}

void mutex_unlock(mutex_t *lock)
{
    futex_unlock(&lock->futex);
}
