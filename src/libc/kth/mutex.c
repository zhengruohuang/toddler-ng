#include <kth.h>
#include <sys.h>
#include <futex.h>
#include <assert.h>


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

void mutex_lock_spin(mutex_t *lock, int max_spin)
{
    futex_lock(&lock->futex, max_spin);
}

int mutex_trylock(mutex_t *lock)
{
    return futex_trylock(&lock->futex);
}

void mutex_unlock(mutex_t *lock)
{
    futex_unlock(&lock->futex);
}
