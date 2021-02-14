#include "common/include/inttypes.h"
#include "common/include/compiler.h"
#include "common/include/atomic.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/dev.h"


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
static volatile ulong rwlock = 0; // { 31 = update, 30:0 = num reads }
static u64 ms = 0;

u64 clock_get_ms()
{
    u64 copy = 0;
    ulong old_rwlock, new_rwlock;

    do {
        old_rwlock = rwlock;
        while (old_rwlock >> 31) {
            atomic_pause();
            atomic_mb();
            old_rwlock = rwlock;
        }

        new_rwlock = old_rwlock + 0x1ul;
    } while(!atomic_cas_bool(&rwlock, old_rwlock, new_rwlock));

    atomic_mb();
    copy = ms;
    atomic_mb();
    atomic_dec(&rwlock);

    return copy;
}

void clock_set_ms(int clk_idx, u64 new_ms)
{
    if (clk_idx != best_clk_idx) {
        return;
    }

    ulong old_rwlock, new_rwlock = 0x1ul << 31;

    do {
        old_rwlock = rwlock;
        while (old_rwlock) {
            atomic_pause();
            atomic_mb();
            old_rwlock = rwlock;
        }
    } while(!atomic_cas_bool(&rwlock, old_rwlock, new_rwlock));

    atomic_mb();
    ms = new_ms;
    atomic_mb();

    atomic_cas_bool(&rwlock, new_rwlock, 0);
}

void clock_advance_ms(int clk_idx, ulong advance)
{
    if (clk_idx != best_clk_idx) {
        return;
    }

    ulong old_rwlock, new_rwlock = 0x1ul << 31;

    do {
        old_rwlock = rwlock;
        while (old_rwlock) {
            atomic_pause();
            atomic_mb();
            old_rwlock = rwlock;
        }
    } while(!atomic_cas_bool(&rwlock, old_rwlock, new_rwlock));

    atomic_mb();
    ms += (u64)advance;
    atomic_mb();

    atomic_cas_bool(&rwlock, new_rwlock, 0);
}
