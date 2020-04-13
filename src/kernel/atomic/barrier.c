/*
 * Barrier
 */


#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "kernel/include/kernel.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"


void barrier_init(barrier_t *barrier, ulong total)
{
    barrier->local = NULL;
    barrier->total = total;
    atomic_mb();
}

void barrier_destroy(barrier_t *barrier)
{
    barrier->local = NULL;
    barrier->total = 0;
    atomic_mb();
}

// #include "kernel/include/kernel.h"

void barrier_wait(barrier_t *barrier)
{
    if (barrier->total <= 1) {
        return;
    }

    local_barrier_t local;
    local.count = local.release = 0;
    //atomic_mb();

    local_barrier_t *my_local = &local;
    local_barrier_t *other_local = (void *)atomic_cas_val(
            (volatile ulong *)&barrier->local, (ulong)NULL, (ulong)my_local);
    atomic_mb();

    int first = 0;
    if (!other_local) {
        first = 1;
    } else {
        my_local = other_local;
    }

    atomic_inc(&my_local->count);

    //kprintf("seq: %d, barrier @ %p, my_local @ %p, other_local @ %p, count: %ld, total: %ld, first: %d\n",
    //        hal_get_cur_mp_seq(), barrier, my_local, other_local, my_local->count, barrier->total, first);

    if (first) {
        while (my_local->count != barrier->total) {
            atomic_pause();
        }
        atomic_notify();

        barrier->local = NULL;
        atomic_mb();
        my_local->release = 1;

        while (my_local->count != 1) {
            atomic_pause();
        }
        atomic_notify();
    } else {
        while (!my_local->release) {
            atomic_pause();
        }
        atomic_notify();

        atomic_dec(&my_local->count);
    }
}

void barrier_wait_int(barrier_t *barrier)
{
    int enabled = hal_disable_local_int();
    barrier_wait(barrier);
    hal_restore_local_int(enabled);
}
