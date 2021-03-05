/*
 * Wait event
 */


#include "common/include/inttypes.h"
#include "kernel/include/atomic.h"
#include "kernel/include/syscall.h"
#include "kernel/include/proc.h"


void event_init(struct wait_event *e, int max_spin)
{
    e->futex.value = 0; // FIXME
    e->max_spin = max_spin;
}

static inline void _wait_for_signal(struct wait_event *e)
{
    int max_spin = e->max_spin;
    futex_t *f = &e->futex;
    futex_t f_old, f_new;

    int num_spins = 0;
    while (max_spin < 0 || num_spins++ <= max_spin) {
        // Already signaled
        if (f->locked) {
            return;
        }
    }

    do {
        f_old.value = f->value;
        if (f_old.locked) {
            return;
        }

        f_new.value = f_old.value;
        f_new.kernel = 1;
    } while (!atomic_cas_bool(&f->value, f_old.value, f_new.value));

    if (f_new.kernel) {
        syscall_wait_on_futex(f, FUTEX_WHEN_EQ | 0);
    }
}

static inline void _reset_signal(struct wait_event *e)
{
    futex_t *f = &e->futex;
    futex_t f_old, f_new;

    do {
        f_old.value = f->value;
        if (!f_old.locked) {
            return;
        }

        f_new.value = f_old.value;
        f_new.locked = 0;
    } while (!atomic_cas_bool(&f->value, f_old.value, f_new.value));
}

int event_wait(struct wait_event *e)
{
    _wait_for_signal(e);
    _reset_signal(e);
    return 0;

}

int event_signal(struct wait_event *e)
{
    futex_t *f = &e->futex;
    futex_t f_old, f_new;

    do {
        f_old.value = f->value;
        f_new.value = f_old.value;
        f_new.locked = 1;
    } while (!atomic_cas_bool(&f->value, f_old.value, f_new.value));

    if (f_new.kernel) {
        do {
            f_old.value = f->value;
            f_new.value = f_old.value;
            if (!f_new.kernel) {
                return 0;
            }

            f_new.kernel = 0;
        } while (!atomic_cas_bool(&f->value, f_old.value, f_new.value));

        wake_on_object_exclusive(get_kernel_proc(), NULL, WAIT_ON_FUTEX,
                                 (ulong)f, FUTEX_WHEN_EQ | 0x1ul, 0);

        //syscall_wake_on_futex(f, FUTEX_WHEN_EQ | 0x1ul);
    }

    return 0;
}
