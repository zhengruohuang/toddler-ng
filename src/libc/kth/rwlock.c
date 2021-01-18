#include <stdio.h>
#include <atomic.h>
#include <assert.h>
#include <kth.h>
#include <sys.h>


/*
 * LSB of futex "locked" field indicates if there is a write,
 * either waiting for the lock or already holding it
 */
void rwlock_init(rwlock_t *lock)
{
    futex_init(&lock->rd_futex);
    futex_init(&lock->wr_futex);
    futex_init(&lock->wait_futex);
    lock->max_spin = RWLOCK_MAX_SPIN;
}


/*
 * Read lock
 */
void rwlock_rlock(rwlock_t *lock)
{
    volatile futex_t f_old, f_new;

    do {
        do {
            // Check and wait for on-going write
            futex_wait(&lock->wait_futex, lock->max_spin);

            // Try inc reader count
            f_old.value = lock->rd_futex.value;

        // A writer is registered, check wait lock again
        } while (f_old.locked & 0x1ul);

        f_new.value = f_old.value;
        f_new.locked += 0x2ul;
    } while (!atomic_cas(&lock->rd_futex.value, f_old.value, f_new.value));

    atomic_mb();
}

int rwlock_rtrylock(rwlock_t *lock)
{
    futex_t f_old, f_new;

    f_old.value = lock->rd_futex.value;
    if (f_old.locked & 0x1ul) {
        // A writer has registered
        return -1;
    }

    f_new.value = f_old.value;
    f_new.locked += 0x2ul;

    int ok = atomic_cas(&lock->rd_futex.value, f_old.value, f_new.value);
    atomic_mb();
    return ok ? 0 : -1;
}

void rwlock_runlock(rwlock_t *lock)
{
    futex_t f_old, f_new;

    atomic_mb();

    // Dec read counter
    do {
        f_old.value = lock->rd_futex.value;
        panic_if(f_old.locked <= 0x1ul,
                 "Inconsistent rwlock state: %lu\n", f_old.locked);

        f_new.value = f_old.value;
        f_new.locked -= 0x2ul;
    } while (!atomic_cas(&lock->rd_futex.value, f_old.value, f_new.value));

    // Wakeup writer
    if (f_new.locked == 0x1ul) {
        // Check if syscall is required, and clear kernel flag
        do {
            f_old.value = lock->rd_futex.value;
            if (!f_old.kernel) {
                break;
            }

            f_new.value = f_old.value;
            f_new.kernel = 0;
        } while (!atomic_cas(&lock->rd_futex.value, f_old.value, f_new.value));

        if (f_old.kernel) {
            syscall_wake_on_futex(&lock->rd_futex, FUTEX_WHEN_NE | 0);
        }
    }
}


/*
 * Write lock
 */
static inline void wait_for_reads(futex_t *f, const int spin)
{
    futex_t f_old, f_new;

    // Set LSB of f->lock to 1, indicating a write is waiting
    do {
        atomic_mb();

        f_old.value = f->value;
        panic_if(f_old.locked & 0x1ul, "Inconsistent rwlock state!\n");

        f_new.value = f_old.value;
        f_new.locked += 0x1ul;  // f_old.locked | 0x1ul;
    } while (!atomic_cas(&f->value, f_old.value, f_new.value));

    // Wait for on-going readers to finish
    int acquired = 0;
    while (!acquired) {
        int num_spins = 0;
        do {
            atomic_mb();

            f_old.value = f->value;
            f_new.value = f_old.value;
            if (f_new.locked != 0x1ul) {
                acquired = 0;
                if (spin >= 0 || ++num_spins > spin) {
                    f_new.kernel = 1;
                }
            } else {
                acquired = 1;
                f_new.kernel = 0;
            }
        } while (!atomic_cas(&f->value, f_old.value, f_new.value));

        if (!acquired && f_new.kernel) {
            syscall_wait_on_futex(f, FUTEX_WHEN_NE | 0x1ul);
        }
    }
}

void rwlock_wlock(rwlock_t *lock)
{
    // Acquire writer lock, so that only one writer is allowed
    futex_lock(&lock->wr_futex, lock->max_spin);

    // Acquire read waiter lock, so that future readers must wait
    // Note that this must not fail
    int err = futex_trylock(&lock->wait_futex);
    panic_if(err, "Inconsistent rwlock state!\n");

    // Now wait for on-going readers
    wait_for_reads(&lock->rd_futex, lock->max_spin);
}

int rwlock_wtrylock(rwlock_t *lock)
{
    int err = futex_trylock(&lock->wr_futex);
    if (!err) {
        return -1;
    }

    futex_t f_old, f_new;

    // Set LSB of f->lock to 1, indicating a write is waiting
    f_old.value = lock->rd_futex.value;
    panic_if(f_old.locked & 0x1ul, "Inconsistent rwlock state!\n");

    // Fail when there are on-going readers
    if (f_old.locked) {
        futex_unlock(&lock->wr_futex);
        return -1;
    }

    // Try locking
    f_new.value = f_old.value;
    f_new.locked |= 0x1ul;

    int ok = atomic_cas(&lock->rd_futex.value, f_old.value, f_new.value);
    if (!ok) {
        futex_unlock(&lock->wr_futex);
        return -1;
    }

    // OK
    return 0;
}

void rwlock_wunlock(rwlock_t *lock)
{
    futex_t f_old, f_new;

    f_old.value = lock->rd_futex.value;
    panic_if(f_old.locked != 0x1ul, "Inconsistent rwlock state: %lu\n", f_old.locked);

    f_new.value = f_old.value;
    //f_new.kernel = 0;
    f_new.locked = 0;
    int ok = atomic_cas(&lock->rd_futex.value, f_old.value, f_new.value);
    panic_if(!ok, "Inconsistent rwlock state: %lu\n", f_old.locked);

    futex_unlock(&lock->wait_futex);
    futex_unlock(&lock->wr_futex);
}


/*
 * Upgrade
 */
void rwlock_upgrade(rwlock_t *lock)
{
    panic("Not implemented!\n");
}

int rwlock_tryupgrade(rwlock_t *lock)
{
    panic("Not implemented!\n");
    return -1;
}

void rwlock_downgrade(rwlock_t *lock)
{
    panic("Not implemented!\n");
}
