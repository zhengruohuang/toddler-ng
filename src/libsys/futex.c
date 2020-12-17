#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/proc.h"
#include "system/include/kprintf.h"
#include "libsys/include/syscall.h"
#include "libk/include/debug.h"


int futex_init(futex_t *f)
{
    f->value = 0;
    f->valid = 1;
    atomic_mb();

    return 0;
}

int futex_destory(futex_t *f)
{
    atomic_mb();
    f->valid = 0;
    atomic_mb();

    return 0;
}

int futex_lock(futex_t *f, int spin)
{
    futex_t f_old, f_new;

    for (int acquired = 0; !acquired; ) {
        int num_spins = 0;
        do {
            f_old.value = f->value;
            f_new.value = f_old.value;
            if (f_new.locked) {
                acquired = 0;
                if (!spin || ++num_spins > spin) {
                    f_new.kernel = 1;
                }
            } else {
                acquired = 1;
                f_new.locked = 1;
            }
        } while (!atomic_cas(&f->value, f_old.value, f_new.value));

        if (!acquired && !spin) {
            syscall_wait_on_futex(f, 0);
        }
    }

    return 0;
}

int futex_trylock(futex_t *f)
{
    futex_t f_old, f_new;

    f_old.value = f->value;
    f_new.value = f_old.value;
    if (f_new.locked) {
        return -1;
    } else {
        f_new.locked = 1;
    }

    int success = atomic_cas(&f->value, f_old.value, f_new.value);
    return success ? 0 : -1;
}

int futex_unlock(futex_t *f)
{
    futex_t f_old, f_new;

    do {
        f_old.value = f->value;
        f_new.value = f_old.value;
        if (!f_new.locked) {
            return -1;
        }

        f_new.locked = 0;
    } while (!atomic_cas(&f->value, f_old.value, f_new.value));

    if (f_new.kernel) {
        do {
            f_old.value = f->value;
            f_new.value = f_old.value;
            if (f_new.locked || !f_new.kernel) {
                return 0;
            }

            f_new.kernel = 0;
        } while (!atomic_cas(&f->value, f_old.value, f_new.value));

        syscall_wake_on_futex(f, 1);
    }

    return 0;
}

int futex_arrive(futex_t *f, ulong total, int spin)
{
}
