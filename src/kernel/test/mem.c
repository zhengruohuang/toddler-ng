#include "common/include/inttypes.h"
#include "kernel/include/atomic.h"
#include "kernel/include/kernel.h"
#include "kernel/include/mem.h"


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

    if (!mp_seq) {
        kprintf("Testing palloc\n");
    }

    barrier_wait_int(bar);

    int i, j, k;
    ppfn_t results[PALLOC_TEST_ORDER_COUNT][PALLOC_TEST_PER_ORDER];

    for (k = 0; k < PALLOC_TEST_LOOPS; k++) {
        for (i = 0; i < PALLOC_TEST_ORDER_COUNT; i++) {
            for (j = 0; j < PALLOC_TEST_PER_ORDER; j++) {
                int count = 0x1 << i;
                ppfn_t pfn = palloc(count);
                results[i][j] = pfn;
                //kprintf("Allocated %d pages @ PFN: %llx\n", count, (u64)pfn);
            }
        }

        for (i = 0; i < PALLOC_TEST_ORDER_COUNT; i++) {
            for (j = 0; j < PALLOC_TEST_PER_ORDER; j++) {
                ppfn_t pfn = results[i][j];
                if (pfn) {
                    //int count = 0x1 << i;
                    //kprintf("Freeing count: %d, index: %d, PFN: %p\n", count, j, pfn);
                    pfree(pfn);
                }
            }
        }
    }

    barrier_wait_int(bar);

    if (!mp_seq) {
        buddy_print();
        kprintf("Done testing palloc\n");
    }

    barrier_wait_int(bar);
}


/*
 * Entry
 */
static barrier_t bar;

void test_mem()
{
    int mp_seq = hal_get_cur_mp_seq();

    test_mem_palloc(mp_seq, &bar);
}

void init_test()
{
    int num_cpus = hal_get_num_cpus();
    barrier_init(&bar, num_cpus);
}
