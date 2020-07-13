/*
 * Readers-writer lock
 */

#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "kernel/include/kernel.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"


/*
 * Init
 */
void rwlock_init(rwlock_t *lock)
{
    lock->value = 0;
}


/*
 * No int-state save and restore
 */
void rwlock_read_lock(rwlock_t *lock)
{
    rwlock_t old_val, new_val;

    do {
        do {
            atomic_pause();
        } while (lock->write);
        atomic_mb();

        new_val.value = old_val.value = lock->value;
        new_val.write = 0;
        new_val.reads++;
    } while(!atomic_cas(&lock->value, old_val.value, new_val.value));

    atomic_mb();
}

void rwlock_read_unlock(rwlock_t *lock)
{
    rwlock_t old_val, new_val;
    atomic_mb();

    do {
        assert(!lock->write);
        new_val.value = old_val.value = lock->value;
        new_val.reads--;
    } while(!atomic_cas(&lock->value, old_val.value, new_val.value));

    atomic_mb();
    atomic_notify();
}

void rwlock_write_lock(rwlock_t *lock)
{
    rwlock_t new_val = RWLOCK_INIT;

    do {
        do {
            atomic_pause();
        } while (lock->reads);
        atomic_mb();

        new_val.write = 1;
        new_val.reads = 0;
    } while(!atomic_cas(&lock->value, 0, new_val.value));

    atomic_mb();
}

void rwlock_write_unlock(rwlock_t *lock)
{
    atomic_mb();
    lock->value = 0;
    atomic_mb();
    atomic_notify();
}


/*
 * With ini-state save and resotre
 */
int rwlock_read_lock_int(rwlock_t *lock)
{
    int enabled = hal_disable_local_int();
    rwlock_read_lock(lock);
    return enabled;
}

void rwlock_read_unlock_int(rwlock_t *lock, int enabled)
{
    rwlock_read_unlock(lock);
    hal_restore_local_int(enabled);
}

void rwlock_write_lock_int(rwlock_t *lock)
{
    int enabled = hal_disable_local_int();

    rwlock_write_lock(lock);

    rwlock_t oldlock, newlock;
    oldlock.value = newlock.value = lock->value;
    newlock.int_enabled = enabled;

    assert(!newlock.reads);
    assert(newlock.write);

    int success = atomic_cas(&lock->value, oldlock.value, newlock.value);
    assert(success);
    atomic_mb();
}

void rwlock_write_unlock_int(rwlock_t *lock)
{
    int enabled = lock->int_enabled;
    rwlock_write_unlock(lock);
    hal_restore_local_int(enabled);
}
