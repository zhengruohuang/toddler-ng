#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"


/*
 * Helpers
 */
static void random_delay(u32 *rand_state, int order)
{
}


/*
 * Mutex
 */
#define MAX_NUM_MUTEX_THREADS   64
#define NUM_MUTEX_LOOPS         4096

static kthread_t mutex_threads[MAX_NUM_MUTEX_THREADS];
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

static void test_mutex()
{
    kprintf("Testing mutex\n");
    mutex_init(&mutex);

    for (int num_threads = 1; num_threads <= MAX_NUM_MUTEX_THREADS; num_threads <<= 1) {
        kprintf("num_threads: %d\n", num_threads);

        ulong loop_count = NUM_MUTEX_LOOPS;

        mutex_counter = 0;
        atomic_mb();

        for (int i = 0; i < num_threads; i++) {
            kthread_create(&mutex_threads[i], test_mutex_worker, loop_count);
        }
        for (int i = 0; i < num_threads; i++) {
            kthread_join(&mutex_threads[i], NULL);
        }

        ulong check_counter = num_threads * NUM_MUTEX_LOOPS;
        if (mutex_counter != check_counter) {
            kprintf_unlocked("Bad counter!\n");
        }
    }

    kprintf("Passed mutex test!\n");
}


/*
 * RW-lock
 */
#define NUM_RWLOCK_THREADS      64
#define RWLOCK_WR_THREAD_MOD    2
#define NUM_RWLOCK_LOOPS        4096

static kthread_t rwlock_threads[NUM_RWLOCK_THREADS];
static fast_rwlock_t rwlock = FAST_RWLOCK_INITIALIZER;
static volatile ulong rwlock_counter = 0;

static ulong test_rwlock_worker(ulong param)
{
    ulong tid = syscall_get_tib()->tid;

    u32 rand_state = tid;
    int rd = param;

    for (ulong i = 0; i < NUM_RWLOCK_LOOPS; i++) {
        if (rd) {
            fast_rwlock_rdlock(&rwlock);
            random_delay(&rand_state, 10);
            fast_rwlock_rdunlock(&rwlock);
        } else {
            fast_rwlock_wrlock(&rwlock);
            rwlock_counter++;
            atomic_mb();
            random_delay(&rand_state, 10);
            fast_rwlock_wrunlock(&rwlock);
        }
    }

    return 0;
}

static void test_rwlock()
{
    kprintf("Testing rwlock\n");
    fast_rwlock_init(&rwlock);

    rwlock_counter = 0;
    atomic_mb();

    ulong num_wr_threads = 0;
    for (int i = 0; i < NUM_RWLOCK_THREADS; i++) {
        if (!(i % RWLOCK_WR_THREAD_MOD)) {
            kthread_create(&rwlock_threads[i], test_rwlock_worker, 1);
            num_wr_threads++;
        } else {
            kthread_create(&rwlock_threads[i], test_rwlock_worker, 0);
        }
    }

    for (int i = 0; i < NUM_RWLOCK_THREADS; i++) {
        kthread_join(&rwlock_threads[i], NULL);
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
    test_mutex();
    test_rwlock();
}
