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
    } while (!atomic_cas(&lock->value, old_val.value, new_val.value));

    atomic_mb();

    //kprintf("Locked: %p\n", lock);
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

    int success = atomic_cas(&lock->value, old_val.value, new_val.value);
    assert(success);
    atomic_mb();
}

void spinlock_unlock_int(spinlock_t *lock)
{
    int enabled = lock->int_enabled;

    spinlock_unlock(lock);

    //kprintf("spin unlock @ %p, int: %d\n", lock, enabled);

    hal_restore_local_int(enabled);
}
