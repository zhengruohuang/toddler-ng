#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"
#include "libk/include/debug.h"


#define MUTEX_MAX_SPIN_COUNT 100


void mutex_init(mutex_t *lock)
{
    lock->state.value = 0;
    lock->wait_obj_id = syscall_wait_obj_alloc((void *)lock, 1, 0);
    atomic_mb();

    kprintf_unlocked("mutex wait obj id: %lx\n", lock->wait_obj_id);
}

void mutex_destroy(mutex_t *lock)
{
    atomic_mb();
    lock->state.value = 0;
    lock->wait_obj_id = 0;
    // TODO: free wait obj
}

void mutex_lock(mutex_t *lock)
{
    mutex_state_t old_val, new_val;

    do {
        int num_spins = 0;

        atomic_mb();
        old_val.value = lock->state.value;

        while (old_val.locked) {
            atomic_pause();

            atomic_mb();
            old_val.value = lock->state.value;

            if (MUTEX_MAX_SPIN_COUNT && lock->wait_obj_id &&
                old_val.locked && ++num_spins >= MUTEX_MAX_SPIN_COUNT
            ) {
                num_spins = 0;
                syscall_wait_on_semaphore(lock->wait_obj_id);
                old_val.value = lock->state.value;
            }
        }

        atomic_mb();
        assert(!old_val.locked);

        new_val.value = old_val.value;
        new_val.locked = 1;
    } while (!atomic_cas(&lock->state.value, old_val.value, new_val.value));

    atomic_mb();
}

int mutex_trylock(mutex_t *lock)
{
    mutex_state_t old_val;
    atomic_mb();
    old_val.value = lock->state.value;
    if (old_val.locked) {
        return 0;
    }

    mutex_state_t new_val;
    new_val.value = old_val.value;
    new_val.locked = 1;

    return atomic_cas(&lock->state.value, old_val.value, new_val.value);
}

void mutex_unlock(mutex_t *lock)
{
    atomic_mb();

    mutex_state_t old_val;
    old_val.value = lock->state.value;
    assert(old_val.locked);

    mutex_state_t new_val;
    new_val.value = old_val.value;
    new_val.locked = 0;

    int success = atomic_cas(&lock->state.value, old_val.value, new_val.value);
    assert(success);

    atomic_mb();
    atomic_notify();

    if (lock->wait_obj_id) {
        syscall_wake_on_semaphore(lock->wait_obj_id, 0);
    }
}
