#include "common/include/inttypes.h"
#include "kernel/include/atomic.h"
#include "kernel/include/kernel.h"
#include "kernel/include/mem.h"


/*
 * Helpers
 */
static void *alloc_result_buf(ulong size)
{
    ulong result_buf_size = align_up_vsize(size, PAGE_SIZE);
    ulong result_buf_pages = get_vpage_count(result_buf_size);

    void *result_buf = palloc_ptr(result_buf_pages);
    return result_buf;
}

static void free_result_buf(void *result_buf)
{
    pfree_ptr(result_buf);
}


/*
 * Palloc
 */
#define PALLOC_TEST_ORDER_COUNT 8
#define PALLOC_TEST_PER_ORDER   10
#define PALLOC_TEST_LOOPS       1000

static void test_mem_palloc(int mp_seq, barrier_t *bar)
{
    //kprintf("seq: %d\n", mp_seq);

    barrier_wait_int(bar);
    ppfn_t *results = alloc_result_buf(
        sizeof(ppfn_t) * PALLOC_TEST_ORDER_COUNT * PALLOC_TEST_PER_ORDER);

    barrier_wait_int(bar);
    if (!mp_seq) {
        kprintf("Testing palloc\n");
    }

    barrier_wait_int(bar);
    for (int k = 0; k < PALLOC_TEST_LOOPS; k++) {
        for (int i = 0, r = 0; i < PALLOC_TEST_ORDER_COUNT; i++) {
            for (int j = 0; j < PALLOC_TEST_PER_ORDER; j++) {
                int count = 0x1 << i;
                ppfn_t pfn = palloc(count);
                if (!pfn) {
                    kprintf("Palloc test failed!\n");
                }
                results[r++] = pfn;
            }
        }

        for (int i = 0, r = 0; i < PALLOC_TEST_ORDER_COUNT; i++) {
            for (int j = 0; j < PALLOC_TEST_PER_ORDER; j++) {
                ppfn_t pfn = results[r++];
                if (pfn) {
                    pfree(pfn);
                }
            }
        }
    }

    barrier_wait_int(bar);
    free_result_buf(results);

    barrier_wait_int(bar);
    if (!mp_seq) {
        kprintf("Done testing palloc\n");
    }
}


/*
 * Malloc
 */
#define MALLOC_TEST_PER_SIZE    100
#define MALLOC_TEST_LOOPS       100

static int test_sizes[] = { 16, 32, 64, 128, 256, 384, 512, 768 };
#define MALLOC_NUM_TEST_SIZES (sizeof(test_sizes) / sizeof(int))

static void test_mem_malloc(int mp_seq, barrier_t *bar)
{
    barrier_wait_int(bar);
    void **results = alloc_result_buf(
        sizeof(void *) * MALLOC_NUM_TEST_SIZES * MALLOC_TEST_PER_SIZE);

    barrier_wait_int(bar);
    if (!mp_seq) {
        kprintf("Testing malloc\n");
    }

    barrier_wait_int(bar);
    for (int k = 0; k < MALLOC_TEST_LOOPS; k++) {
        for (int i = 0, r = 0; i < MALLOC_NUM_TEST_SIZES; i++) {
            size_t size = test_sizes[i];

            for (int j = 0; j < MALLOC_TEST_PER_SIZE; j++) {
                void *ptr = malloc(size);
                if (!ptr) {
                    kprintf("Malloc test failed!\n");
                }
                results[r++] = ptr;
            }
        }

        for (int i = 0, r = 0; i < MALLOC_NUM_TEST_SIZES; i++) {
            for (int j = 0; j < MALLOC_TEST_PER_SIZE; j++) {
                void *ptr = results[r++];
                if (ptr) {
                    free(ptr);
                }
            }
        }
    }

    barrier_wait_int(bar);
    free_result_buf(results);

    barrier_wait_int(bar);
    if (!mp_seq) {
        kprintf("Done testing malloc\n");
    }
}


/*
 * Entry
 */
static barrier_t bar;

void test_mem()
{
    int mp_seq = hal_get_cur_mp_seq();

    barrier_wait_int(&bar);
    if (!mp_seq) {
        kprintf("Before mem testing\n");
        buddy_print();
    }

    barrier_wait_int(&bar);
    test_mem_palloc(mp_seq, &bar);
    test_mem_malloc(mp_seq, &bar);

    barrier_wait_int(&bar);
    if (!mp_seq) {
        kprintf("After mem testing\n");
        buddy_print();
    }

    barrier_wait_int(&bar);
}

void init_test()
{
    int num_cpus = hal_get_num_cpus();
    barrier_init(&bar, num_cpus);
}
