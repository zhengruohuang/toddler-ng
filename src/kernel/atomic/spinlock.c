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
    do {
        do {
            atomic_pause();
        } while (lock->value);
        atomic_mb();
    } while (!atomic_cas(&lock->value, 0, 1));

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

    spinlock_t newlock;
    newlock.locked = 1;
    newlock.int_enabled = enabled;

    int success = atomic_cas(&lock->value, 1, newlock.value);
    assert(success);
    atomic_mb();
}

void spinlock_unlock_int(spinlock_t *lock)
{
    int enabled = lock->int_enabled;

    spinlock_unlock(lock);

//     if (enabled) {
//         kprintf("spin store: %d\n", enabled);
//     }

    hal_restore_local_int(enabled);
}
