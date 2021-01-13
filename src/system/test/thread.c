#include <stdio.h>
#include <futex.h>
#include <sys.h>
#include <kth.h>

#include "common/include/inttypes.h"


/*
 * Helpers
 */
static void random_delay(u32 *rand_state, int order)
{
}


/*
 * Futex
 */
#define MAX_NUM_FUTEX_THREADS   16
#define NUM_FUTEX_LOOPS         163840

static kth_t futex_threads[MAX_NUM_FUTEX_THREADS];
static futex_t futex = FUTEX_INITIALIZER;
static volatile ulong futex_counter = 0;

static ulong test_futex_worker(ulong param)
{
    ulong tid = syscall_get_tib()->tid;

    u32 rand_state = tid;
    ulong count = param;

    for (ulong i = 0; i < count; i++) {
        futex_exclusive(&futex) {
            futex_counter++;
            atomic_mb();
            random_delay(&rand_state, 10);
        }
    }

    return 0;
}

__unused_func static void test_futex()
{
    kprintf("Testing Futex\n");
    futex_init(&futex);

    int fail_count = 0;

    for (int num_threads = 1; num_threads <= MAX_NUM_FUTEX_THREADS; num_threads <<= 1) {
        kprintf("num_threads: %d\n", num_threads);

        ulong loop_count = NUM_FUTEX_LOOPS;

        futex_counter = 0;
        atomic_mb();

        for (int i = 0; i < num_threads; i++) {
            kth_create(&futex_threads[i], test_futex_worker, loop_count);
        }
        for (int i = 0; i < num_threads; i++) {
            kth_join(&futex_threads[i], NULL);
        }

        ulong check_counter = num_threads * NUM_FUTEX_LOOPS;
        if (futex_counter != check_counter) {
            kprintf_unlocked("Bad counter!\n");
            fail_count++;
        }
    }

    if (!fail_count) {
        kprintf("Passed Futex tests!\n");
    } else {
        kprintf("Failed %d Futex tests!\n", fail_count);
    }
}


/*
 * Mutex
 */
#define MAX_NUM_MUTEX_THREADS   16
#define NUM_MUTEX_LOOPS         163840

static kth_t mutex_threads[MAX_NUM_MUTEX_THREADS];
static mutex_t mutex = MUTEX_INITIALIZER;
static volatile ulong mutex_counter = 0;

static ulong test_mutex_worker(ulong param)
{
    ulong tid = syscall_get_tib()->tid;

    u32 rand_state = tid;
    ulong count = param;

    for (ulong i = 0; i < count; i++) {
        mutex_exclusive(&mutex) {
            mutex_counter++;
            atomic_mb();
            random_delay(&rand_state, 10);
        }
    }

    return 0;
}

__unused_func static void test_mutex()
{
    kprintf("Testing Mutex\n");
    mutex_init(&mutex);

    int fail_count = 0;

    for (int num_threads = 1; num_threads <= MAX_NUM_MUTEX_THREADS; num_threads <<= 1) {
        kprintf("num_threads: %d\n", num_threads);

        ulong loop_count = NUM_MUTEX_LOOPS;

        mutex_counter = 0;
        atomic_mb();

        for (int i = 0; i < num_threads; i++) {
            kth_create(&mutex_threads[i], test_mutex_worker, loop_count);
        }
        for (int i = 0; i < num_threads; i++) {
            kth_join(&mutex_threads[i], NULL);
        }

        ulong check_counter = num_threads * NUM_MUTEX_LOOPS;
        if (mutex_counter != check_counter) {
            kprintf_unlocked("Bad counter!\n");
            fail_count++;
        }
    }

    if (!fail_count) {
        kprintf("Passed Mutex tests!\n");
    } else {
        kprintf("Failed %d Mutex tests!\n", fail_count);
    }
}


/*
 * RW-lock
 */
#define NUM_RWLOCK_THREADS      64
#define RWLOCK_WR_THREAD_MOD    100
#define NUM_RWLOCK_LOOPS        163840

static kth_t rwlock_threads[NUM_RWLOCK_THREADS];
static rwlock_t rwlock = RWLOCK_INITIALIZER;
static volatile ulong rwlock_counter = 0;

static ulong test_rwlock_worker(ulong param)
{
    ulong tid = syscall_get_tib()->tid;

    u32 rand_state = tid;
    int rd = param;

    for (ulong i = 0; i < NUM_RWLOCK_LOOPS; i++) {
        // Reader
        if (rd) {
            rwlock_rlock(&rwlock);
            //kprintf("read counter: %lu\n", rwlock_counter);
            random_delay(&rand_state, 10);
            rwlock_runlock(&rwlock);
        }

        // Write
        else {
            rwlock_wlock(&rwlock);
            rwlock_counter++;
            //kprintf("write counter: %lu\n", rwlock_counter);
            atomic_mb();
            random_delay(&rand_state, 10);
            rwlock_wunlock(&rwlock);
        }
    }

    //if (rd) kprintf("rd done\n");
    //else kprintf("wr done\n");

    return 0;
}

__unused_func static void test_rwlock()
{
    kprintf("Testing rwlock, rd futex @ %p\n", &rwlock.rd_futex);
    rwlock_init(&rwlock);

    rwlock_counter = 0;
    atomic_mb();

    ulong num_wr_threads = 0;
    for (int i = 0; i < NUM_RWLOCK_THREADS; i++) {
        if (!(i % RWLOCK_WR_THREAD_MOD)) {
            kth_create(&rwlock_threads[i], test_rwlock_worker, 0);
            num_wr_threads++;
        } else {
            kth_create(&rwlock_threads[i], test_rwlock_worker, 1);
        }
    }

    for (int i = 0; i < NUM_RWLOCK_THREADS; i++) {
        kth_join(&rwlock_threads[i], NULL);
    }

    ulong check_counter = num_wr_threads * NUM_RWLOCK_LOOPS;
    atomic_mb();
    if (rwlock_counter != check_counter) {
        kprintf("Bad counter!\n");
    }

    kprintf("Passed rwlock test!\n");
}


/*
 * Entry
 */
void test_thread()
{
    //test_futex();
    //test_mutex();
    test_rwlock();
}
