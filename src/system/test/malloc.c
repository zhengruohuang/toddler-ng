#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/stdlib.h"
#include "libk/include/string.h"
#include "libk/include/rand.h"


#define MAX_MALLOC_SIZE 128 //0x400000ul - 128  // 4 MB - 128
#define NUM_SLOTS       8192
#define NUM_ALLOCS      (NUM_SLOTS * 4)

#define NUM_THREADS     16


static ulong malloc_worker(ulong param)
{
    void **ptrs = calloc(NUM_SLOTS, sizeof(void *));
    memzero(ptrs, NUM_SLOTS * sizeof(void *));

    u32 seed = param;
    for (int i = 0; i < NUM_ALLOCS; i++) {
        int idx = rand_r(&seed) % NUM_SLOTS;
        size_t size = rand_r(&seed) % MAX_MALLOC_SIZE;

        // Already allocated
        if (ptrs[idx]) {
            int do_realloc = rand_r(&seed) % 2;
            if (do_realloc) {
                ptrs[idx] = realloc(ptrs[idx], size);
            } else {
                free(ptrs[idx]);
                ptrs[idx] = malloc(size);
            }
        }

        // First time allocation
        else {
            ptrs[idx] = malloc(size);
        }
    }

    // Free all
    for (int idx = 0; idx < NUM_SLOTS; idx++) {
        if (ptrs[idx]) {
            free(ptrs[idx]);
        }
    }

    free(ptrs);
    return 0;
}

static void single_thread_malloc()
{
    malloc_worker(1);
    display_malloc();
}

static void multi_thread_malloc()
{
    kthread_t *threads = calloc(NUM_THREADS, sizeof(kthread_t));

    for (int i = 0; i < NUM_THREADS; i++) {
        kthread_create(&threads[i], malloc_worker, i + 1);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        kthread_join(&threads[i], NULL);
    }

    free(threads);
    display_malloc();
}


/*
 * Entry
 */
void test_malloc()
{
    kprintf("Testing malloc\n");

    single_thread_malloc();
    multi_thread_malloc();

    kprintf("Passed malloc tests!\n");
}
