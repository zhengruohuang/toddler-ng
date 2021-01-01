#include <stdio.h>
#include <atomic.h>
#include <assert.h>
#include <kth.h>
#include <sys.h>


#define FAST_RWLOCK_MAX_SPIN_COUNT 0


void fast_rwlock_init(fast_rwlock_t *lock)
{
    lock->state.value = 0;
    //lock->rd_wait_obj_id = syscall_wait_obj_alloc((void *)lock, 1, 0);
    //lock->wr_wait_obj_id = syscall_wait_obj_alloc((void *)lock, 1, 0);
    lock->rd_wait_obj_id = 0;
    lock->wr_wait_obj_id = 0;
    atomic_mb();

    kprintf_unlocked("rwlock wait obj id, rd: %lx, wr: %lx\n",
                     lock->rd_wait_obj_id, lock->wr_wait_obj_id);
}

void fast_rwlock_destroy(fast_rwlock_t *lock)
{
    atomic_mb();
    lock->state.value = 0;
    lock->rd_wait_obj_id = 0;
    lock->wr_wait_obj_id = 0;
    // TODO: free wait obj
}


/*
 * Reader lock
 */
void fast_rwlock_rdlock(fast_rwlock_t *lock)
{
    fast_rwlock_state_t old_val, new_val;

    do {
        int num_spins = 0;

        atomic_mb();
        old_val.value = lock->state.value;

        if (!old_val.num_readers) {
            atomic_notify();
            if (FAST_RWLOCK_MAX_SPIN_COUNT && lock->wr_wait_obj_id) {
                //syscall_wake_on_semaphore(lock->wr_wait_obj_id, 0);
            }
        }

        while (old_val.locked) {
            atomic_pause();

            atomic_mb();
            old_val.value = lock->state.value;

            if (FAST_RWLOCK_MAX_SPIN_COUNT && lock->rd_wait_obj_id &&
                old_val.locked && ++num_spins >= FAST_RWLOCK_MAX_SPIN_COUNT
            ) {
                num_spins = 0;
                //syscall_wait_on_semaphore(lock->rd_wait_obj_id);
                old_val.value = lock->state.value;
            }
        }

        atomic_mb();
        assert(!old_val.locked);

        new_val.value = old_val.value;
        new_val.num_readers++;
    } while (!atomic_cas(&lock->state.value, old_val.value, new_val.value));

    atomic_mb();
}

int fast_rwlock_rdtrylock(fast_rwlock_t *lock)
{
    fast_rwlock_state_t old_val;
    atomic_mb();
    old_val.value = lock->state.value;
    if (old_val.locked) {
        return 0;
    }

    fast_rwlock_state_t new_val;
    new_val.value = old_val.value;
    new_val.num_readers++;

    int success = atomic_cas(&lock->state.value, old_val.value, new_val.value);
    atomic_mb();

    return success;
}

void fast_rwlock_rdunlock(fast_rwlock_t *lock)
{
    fast_rwlock_state_t old_val, new_val;

    atomic_mb();
    do {
        old_val.value = lock->state.value;
        assert(old_val.num_readers);

        new_val.value = old_val.value;
        new_val.num_readers--;
    } while (!atomic_cas(&lock->state.value, old_val.value, new_val.value));

    atomic_mb();
    atomic_notify();

    if (FAST_RWLOCK_MAX_SPIN_COUNT && lock->wr_wait_obj_id &&
        !new_val.num_readers
    ) {
        //syscall_wake_on_semaphore(lock->wr_wait_obj_id, 0);
    }
}


/*
 * Writer lock
 */
static inline void fast_rwlock_set_locked(fast_rwlock_t *lock)
{
    fast_rwlock_state_t old_val, new_val;

    do {
        int num_spins = 0;

        atomic_mb();
        old_val.value = lock->state.value;

        while (old_val.locked) {
            atomic_pause();

            atomic_mb();
            old_val.value = lock->state.value;

            if (FAST_RWLOCK_MAX_SPIN_COUNT && lock->wr_wait_obj_id &&
                old_val.locked && ++num_spins >= FAST_RWLOCK_MAX_SPIN_COUNT
            ) {
                num_spins = 0;
                //syscall_wait_on_semaphore(lock->wr_wait_obj_id);
                old_val.value = lock->state.value;
            }
        }

        atomic_mb();
        assert(!old_val.locked);

        new_val.value = old_val.value;
        new_val.locked = 1;
    } while (!atomic_cas(&lock->state.value, old_val.value, new_val.value));
}

static inline void fast_rwlock_wait_for_readers(fast_rwlock_t *lock)
{
    fast_rwlock_state_t old_val;

    atomic_mb();
    old_val.value = lock->state.value;

    int num_spins = 0;
    while (old_val.num_readers) {
        atomic_pause();

        atomic_mb();
        old_val.value = lock->state.value;

        if (FAST_RWLOCK_MAX_SPIN_COUNT && lock->wr_wait_obj_id &&
            old_val.locked && ++num_spins >= FAST_RWLOCK_MAX_SPIN_COUNT
        ) {
            num_spins = 0;
            //syscall_wait_on_semaphore(lock->wr_wait_obj_id);
            old_val.value = lock->state.value;
        }
    }
}

void fast_rwlock_wrlock(fast_rwlock_t *lock)
{
    fast_rwlock_set_locked(lock);
    fast_rwlock_wait_for_readers(lock);

    atomic_mb();
}

int fast_rwlock_wrtrylock(fast_rwlock_t *lock)
{
    fast_rwlock_state_t old_val;
    atomic_mb();
    old_val.value = lock->state.value;
    if (old_val.value) {
        return 0;
    }

    fast_rwlock_state_t new_val;
    new_val.value = old_val.value;
    new_val.locked = 1;

    return atomic_cas(&lock->state.value, old_val.value, new_val.value);
}

void fast_rwlock_wrunlock(fast_rwlock_t *lock)
{
    atomic_mb();

    fast_rwlock_state_t old_val;
    old_val.value = lock->state.value;
    assert(old_val.locked);
    assert(!old_val.num_readers);

    fast_rwlock_state_t new_val;
    new_val.value = old_val.value;
    new_val.locked = 0;

    int success = atomic_cas(&lock->state.value, old_val.value, new_val.value);
    assert(success);

    atomic_mb();
    atomic_notify();

    if (FAST_RWLOCK_MAX_SPIN_COUNT) {
        if (lock->rd_wait_obj_id) {
            //syscall_wake_on_semaphore(lock->rd_wait_obj_id, 0);
        }
        if (lock->wr_wait_obj_id) {
            //syscall_wake_on_semaphore(lock->wr_wait_obj_id, 0);
        }
    }
}
