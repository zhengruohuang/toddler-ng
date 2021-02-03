#include <kth.h>
#include <sys.h>
#include <futex.h>
#include <assert.h>


/*
 * Init
 */
void sema_init(sema_t *sema, unsigned long max_count)
{
    futex_init(&sema->futex);
    sema->max_count = max_count;
    sema->max_spin = SEMA_MAX_SPIN;
}

void sema_init_default(sema_t *sema)
{
    futex_init(&sema->futex);
    sema->max_count = 0;
    sema->max_spin = SEMA_MAX_SPIN;
}

void sema_init_spin(sema_t *sema, unsigned long max_count, int max_spin)
{
    futex_init(&sema->futex);
    sema->max_count = max_count;
    sema->max_spin = max_spin;
}

void sema_init_default_spin(sema_t *sema, int max_spin)
{
    futex_init(&sema->futex);
    sema->max_count = 0;
    sema->max_spin = max_spin;
}

void sema_destroy(sema_t *sema)
{
    futex_destory(&sema->futex);
    sema->max_count = 0;
}


/*
 * Wait
 */
int sema_wait(sema_t *sema)
{
    return sema_wait_spin(sema, sema->max_spin);
}

int sema_wait_spin(sema_t *sema, int max_spin)
{
    futex_t *f = &sema->futex;
    futex_t f_old, f_new;

    for (int acquired = 0; !acquired; ) {
        int num_spins = 0;
        do {
            f_old.value = f->value;
            f_new.value = f_old.value;
            if (!f_new.locked) {
                acquired = 0;
                if (max_spin >= 0 || ++num_spins > max_spin) {
                    f_new.kernel = 1;
                }
            } else {
                acquired = 1;
                f_new.locked--;
            }
        } while (!atomic_cas(&f->value, f_old.value, f_new.value));

        if (!acquired && f_new.kernel) {
            syscall_wait_on_futex(f, FUTEX_WHEN_EQ | 0);
        }
    }

    return 0;
}

int sema_trywait(sema_t *sema)
{
    futex_t f_old, f_new;

    f_old.value = sema->futex.value;
    if (!f_old.locked) {
        return -1;
    }

    f_new.value = f_old.value;
    f_new.locked--;

    int success = atomic_cas(&sema->futex.value, f_old.value, f_new.value);
    return success ? 0 : -1;
}


/*
 * Post
 */
int sema_post_count(sema_t *sema, unsigned long count)
{
    futex_t *f = &sema->futex;
    futex_t f_old, f_new;

    do {
        f_old.value = f->value;
        f_new.value = f_old.value;
        f_new.locked += count;
        if (sema->max_count && f_new.locked > sema->max_count) {
            f_new.locked = sema->max_count;
        }
    } while (!atomic_cas(&f->value, f_old.value, f_new.value));

    if (f_new.kernel) {
        do {
            f_old.value = f->value;
            f_new.value = f_old.value;
            if (!f_new.locked || !f_new.kernel) {
                return 0;
            }

            f_new.kernel = 0;
        } while (!atomic_cas(&f->value, f_old.value, f_new.value));

        syscall_wake_on_futex(f, FUTEX_WHEN_NE | 0);
    }

    return 0;
}

int sema_post(sema_t *sema)
{
    return sema_post_count(sema, 1);
}
