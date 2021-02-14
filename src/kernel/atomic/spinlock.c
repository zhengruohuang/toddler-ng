/*
 * Spin lock
 */


#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "kernel/include/kernel.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"


void spinlock_init(spinlock_t *lock)
{
    lock->value = 0;
}

void spinlock_init_locked(spinlock_t *lock)
{
    lock->value = 1;
}

void spinlock_lock(spinlock_t *lock)
{
    spinlock_t old_val, new_val;
    new_val.value = 0;
    new_val.locked = 1;

    do {
        do {
            old_val.value = lock->value;
            atomic_pause();
        } while (old_val.locked);
        atomic_mb();
    } while (!atomic_cas_bool(&lock->value, old_val.value, new_val.value));

    atomic_mb();

    //kprintf("Locked: %p\n", lock);
}

int spinlock_trylock(spinlock_t *lock)
{
    spinlock_t old_val;
    old_val.value = lock->value;
    if (old_val.locked) {
        return -1;
    }

    spinlock_t new_val;
    new_val.value = 0;
    new_val.locked = 1;

    int ok = atomic_cas_bool(&lock->value, old_val.value, new_val.value);
    atomic_mb();

    return ok ? 0 : -1;
}

void spinlock_unlock(spinlock_t *lock)
{
    atomic_mb();

    assert(lock->locked);
    lock->value = 0;

    atomic_mb();
    atomic_notify();

    //kprintf("Unlocked: %p\n", lock);
}

void spinlock_lock_int(spinlock_t *lock)
{
    int enabled = hal_disable_local_int();

    spinlock_lock(lock);

    //kprintf("spin lock @ %p, int: %d\n", lock, enabled);

    spinlock_t old_val;
    old_val.value = lock->value;
    assert(old_val.locked);

    spinlock_t new_val;
    new_val.value = 0;
    new_val.locked = 1;
    new_val.int_enabled = enabled;

    int success = atomic_cas_bool(&lock->value, old_val.value, new_val.value);
    assert(success);
    atomic_mb();
}

int spinlock_trylock_int(spinlock_t *lock)
{
    int enabled = hal_disable_local_int();

    int err = spinlock_trylock(lock);
    if (err) {
        return err;
    }

    spinlock_t old_val;
    old_val.value = lock->value;
    assert(old_val.locked);

    spinlock_t new_val;
    new_val.value = 0;
    new_val.locked = 1;
    new_val.int_enabled = enabled;

    int success = atomic_cas_bool(&lock->value, old_val.value, new_val.value);
    assert(success);
    atomic_mb();

    return 0;
}

void spinlock_unlock_int(spinlock_t *lock)
{
    int enabled = lock->int_enabled;

    spinlock_unlock(lock);

    //kprintf("spin unlock @ %p, int: %d\n", lock, enabled);

    hal_restore_local_int(enabled);
}
