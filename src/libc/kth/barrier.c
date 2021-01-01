#include <stdio.h>
#include <atomic.h>
#include <assert.h>
#include <kth.h>
#include <sys.h>


void barrier_init(barrier_t *bar, ulong total)
{
    bar->local = NULL;
    bar->total = total;
    bar->wait_obj_id = syscall_wait_obj_alloc((void *)bar, 0, 0);
    atomic_mb();

    kprintf_unlocked("mutex wait obj id: %lx\n", bar->wait_obj_id);
}

void barrier_destroy(barrier_t *bar)
{
    atomic_mb();
    bar->local = NULL;
    bar->total = 0;
    bar->wait_obj_id = 0;
    // TODO: free wait obj
}

void barrier_wait(barrier_t *bar)
{
    if (bar->total <= 1) {
        return;
    }

    barrier_local_t local;
    local.count = local.release = 0;

    barrier_local_t *my_local = &local;
    barrier_local_t *other_local = (void *)atomic_cas_val((void *)&bar->local, 0, (ulong)my_local);

    int first = 0;
    if (!other_local) {
        first = 1;
    } else {
        my_local = other_local;
    }

    atomic_inc(&my_local->count);
    atomic_mb();
    atomic_notify();

    if (first) {
        while (my_local->count != bar->total) {
            atomic_pause();
            atomic_mb();
        }

        bar->local = NULL;
        atomic_mb();
        my_local->release = 1;
        atomic_mb();
        atomic_notify();

        while (my_local->count != 1) {
            atomic_pause();
            atomic_mb();
        }
    } else {
        while (!my_local->release) {
            atomic_pause();
            atomic_mb();
        }

        atomic_dec(&my_local->count);
        atomic_mb();
        atomic_notify();
    }
}
