#include <kth.h>
#include <sys.h>
#include <futex.h>
#include <assert.h>


void cond_init(cond_t *cond)
{
    futex_init(&cond->futex);
    cond->max_spin = MUTEX_MAX_SPIN;
}

void cond_init_spin(cond_t *cond, int max_spin)
{
    futex_init(&cond->futex);
    cond->max_spin = max_spin;
}

void cond_destroy(cond_t *cond)
{
    futex_destory(&cond->futex);
}


int cond_wait(cond_t *cond)
{
    return cond_wait_spin(cond, cond->max_spin);
}

int cond_wait_spin(cond_t *cond, int max_spin)
{
    futex_t *f = &cond->futex;
    futex_t f_old, f_new;

    int num_spins = 0;
    while (max_spin < 0 || num_spins <= max_spin) {
        // Already signaled
        if (f->locked) {
            return 0;
        }
    }

    do {
        f_old.value = f->value;
        f_new.value = f_old.value;
        if (f_new.locked) {
            return 0;
        }

        f_new.kernel = 1;
    } while (!atomic_cas(&f->value, f_old.value, f_new.value));

    if (f_new.kernel) {
        syscall_wait_on_futex(f, FUTEX_WHEN_EQ | 0);
    }

    return 0;
}

int cond_wait2(cond_t *cond, mutex_t *mutex)
{
    return cond_wait2_spin(cond, mutex, cond->max_spin);
}

int cond_wait2_spin(cond_t *cond, mutex_t *mutex, int max_spin)
{
    return -1;
}


int cond_signal(cond_t *cond)
{
    futex_t *f = &cond->futex;
    futex_t f_old, f_new;

    do {
        f_old.value = f->value;
        f_new.value = f_old.value;
        f_new.locked = 1;
    } while (!atomic_cas(&f->value, f_old.value, f_new.value));

    if (f_new.kernel) {
        do {
            f_old.value = f->value;
            f_new.value = f_old.value;
            if (!f_new.kernel) {
                return 0;
            }

            f_new.kernel = 0;
        } while (!atomic_cas(&f->value, f_old.value, f_new.value));

        syscall_wake_on_futex(f, FUTEX_WHEN_EQ | 0x1ul);
    }

    return 0;
}
