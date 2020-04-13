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
        } while (lock->locked);

        old_val.value = lock->value;
        new_val.locked = 0;
        new_val.counter = old_val.counter + 1;
    } while(!atomic_cas(&lock->value, old_val.value, new_val.value));

    atomic_notify();
}

void rwlock_read_unlock(rwlock_t *lock)
{
    rwlock_t old_val, new_val;

    do {
        do {
            atomic_pause();
        } while (lock->locked);

        old_val.value = lock->value;
        new_val.locked = 0;
        new_val.counter = old_val.counter - 1;
    } while(!atomic_cas(&lock->value, old_val.value, new_val.value));

    atomic_notify();
}

void rwlock_write_lock(rwlock_t *lock)
{
    rwlock_t new_val;

    do {
        do {
            atomic_pause();
        } while (lock->value);

        new_val.locked = 1;
        new_val.counter = 0;
    } while(!atomic_cas(&lock->value, 0, new_val.value));

    atomic_notify();
}

void rwlock_write_unlock(rwlock_t *lock)
{
    lock->value = 0;
    atomic_notify();
}


/*
 * With ini-state save and resotre
 */
void rwlock_read_lock_int(rwlock_t *lock)
{
    int enabled = hal_disable_local_int();

    rwlock_read_lock(lock);

    lock->int_enabled = enabled;
    atomic_mb();
}

void rwlock_read_unlock_int(rwlock_t *lock)
{
    int enabled = lock->int_enabled;
    rwlock_read_unlock(lock);
    hal_restore_local_int(enabled);
}

void rwlock_write_lock_int(rwlock_t *lock)
{
    int enabled = hal_disable_local_int();

    rwlock_write_lock(lock);

    lock->int_enabled = enabled;
    atomic_mb();
}

void rwlock_write_unlock_int(rwlock_t *lock)
{
    int enabled = lock->int_enabled;
    rwlock_write_unlock(lock);
    hal_restore_local_int(enabled);
}
