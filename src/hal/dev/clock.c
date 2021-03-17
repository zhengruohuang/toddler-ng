#include "common/include/inttypes.h"
#include "common/include/compiler.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


/*
 * RW lock
 */
struct _raw_rwlock {
    union {
        ulong value;

        struct {
            ulong write         : 1;
            ulong num_readers   : sizeof(ulong) * 8 - 1;
        };
    };
} natural_struct;

static inline void _rwlock_rlock(volatile struct _raw_rwlock *l)
{
    struct _raw_rwlock old_rwlock, new_rwlock;

    do {
        atomic_mb();
        old_rwlock.value = l->value;
        while (old_rwlock.write) {
            atomic_pause();
            atomic_mb();
            old_rwlock.value = l->value;
        }

        new_rwlock.value = old_rwlock.value;
        new_rwlock.num_readers++;
    } while (!atomic_cas_bool(&l->value, old_rwlock.value, new_rwlock.value));
}

static inline void _rwlock_runlock(volatile struct _raw_rwlock *l)
{
    struct _raw_rwlock old_rwlock, new_rwlock;

    do {
        atomic_mb();
        old_rwlock.value = l->value;
        panic_if(old_rwlock.write, "Inconsistent RWlock state!\n");
        panic_if(!old_rwlock.num_readers, "Inconsistent RWlock state!\n");

        new_rwlock.value = old_rwlock.value;
        new_rwlock.num_readers--;
    } while (!atomic_cas_bool(&l->value, old_rwlock.value, new_rwlock.value));

    atomic_notify();
}

static inline void _rwlock_wlock(volatile struct _raw_rwlock *l)
{
    struct _raw_rwlock old_rwlock, new_rwlock;

    do {
        atomic_mb();
        old_rwlock.value = l->value;
        while (old_rwlock.num_readers) {
            atomic_pause();
            atomic_mb();
            old_rwlock.value = l->value;
        }

        new_rwlock.value = old_rwlock.value;
        new_rwlock.write = 1;
    } while (!atomic_cas_bool(&l->value, old_rwlock.value, new_rwlock.value));
}

static inline void _rwlock_wunlock(volatile struct _raw_rwlock *l)
{
    struct _raw_rwlock old_rwlock, new_rwlock;

    do {
        atomic_mb();
        old_rwlock.value = l->value;
        panic_if(!old_rwlock.write, "Inconsistent RWlock state!\n");
        panic_if(old_rwlock.num_readers, "Inconsistent RWlock state!\n");

        new_rwlock.value = 0;
    } while (!atomic_cas_bool(&l->value, old_rwlock.value, new_rwlock.value));

    atomic_notify();
}


/*
 * Register
 */
static int num_clocks = 1;

static int best_clk_idx = 0;
static int best_clk_quality = 0;

int clock_source_register(int quality)
{
    int idx = num_clocks++;
    if (quality > best_clk_quality) {
        best_clk_idx = idx;
        best_clk_quality = quality;
    }
    return idx;
}


/*
 * Get/Set/Advance
 */
static volatile struct _raw_rwlock rwlock = { .value = 0 };
static volatile u64 ms = 0;

u64 clock_get_ms()
{
    u64 copy = 0;

    _rwlock_rlock(&rwlock);

    atomic_mb();
    copy = ms;
    atomic_mb();

    _rwlock_runlock(&rwlock);

    return copy;
}

void clock_set_ms(int clk_idx, u64 new_ms)
{
    if (clk_idx != best_clk_idx) {
        return;
    }

    _rwlock_wlock(&rwlock);

    atomic_mb();
    ms = new_ms;
    atomic_mb();

    _rwlock_wunlock(&rwlock);
}

void clock_advance_ms(int clk_idx, ulong advance)
{
    if (clk_idx != best_clk_idx) {
        return;
    }

    _rwlock_wlock(&rwlock);

    atomic_mb();
    ms += (u64)advance;
    atomic_mb();

    _rwlock_wunlock(&rwlock);
}
