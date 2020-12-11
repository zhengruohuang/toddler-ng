/*
 * General Memory Allocator
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"


struct malloc_entry {
    size_t block_size;
    int block_count;
    salloc_obj_t salloc_obj;
};

static struct malloc_entry malloc_entries[] = {
    { 16,   0, SALLOC_OBJ_INIT },
    { 32,   0, SALLOC_OBJ_INIT },
    { 64,   0, SALLOC_OBJ_INIT },
    { 128,  0, SALLOC_OBJ_INIT },
    { 256,  0, SALLOC_OBJ_INIT },
    { 384,  0, SALLOC_OBJ_INIT },
    { 512,  0, SALLOC_OBJ_INIT },
    { 768,  0, SALLOC_OBJ_INIT },
};


void init_malloc()
{
    kprintf("Initializing general memory allocator\n");

    int i;
    int entry_count = sizeof(malloc_entries) / sizeof(struct malloc_entry);

    for (i = 0; i < entry_count; i++) {
        salloc_create(&malloc_entries[i].salloc_obj,
            malloc_entries[i].block_size, 0, malloc_entries[i].block_count,
            NULL, NULL
        );

        // ID == 0 is invalid
        //assert(malloc_entries[i].salloc_id);

        kprintf("\tAlloc obj #%d created, obj @ %p, size: %ld bytes\n", i,
                &malloc_entries[i].salloc_obj, malloc_entries[i].block_size);
    }
}

void *malloc(size_t size)
{
    int i;
    int entry_count = sizeof(malloc_entries) / sizeof(struct malloc_entry);

    for (i = 0; i < entry_count; i++) {
        if (size <= malloc_entries[i].block_size) {
            void *ptr = salloc(&malloc_entries[i].salloc_obj);
            if (ptr) {
                return ptr;
            }
        }
    }

    panic("Out of memory or too big for malloc: %p", size);
    return NULL;
}

void *calloc(ulong count, size_t size)
{
    return malloc(count * size);
}

void free(void *ptr)
{
    sfree(ptr);
}


/*
 * Testing
 */
#define MALLOC_TEST_SIZES       15
#define MALLOC_TEST_STEP        16
#define MALLOC_TEST_PER_SIZE    100
#define MALLOC_TEST_LOOPS       1

void test_malloc()
{
    kprintf("Testing malloc\n");

    int i, j, k;
    ulong results[MALLOC_TEST_SIZES][MALLOC_TEST_PER_SIZE];

    for (k = 0; k < MALLOC_TEST_LOOPS; k++) {
        for (i = 0; i < MALLOC_TEST_SIZES; i++) {
            size_t size = MALLOC_TEST_STEP * (i + 1);

            for (j = 0; j < MALLOC_TEST_PER_SIZE; j++) {
//                 kprintf("\tAllocating size: %d, index: %d", size, j);
                ulong result = (ulong)malloc(size);
                results[i][j] = result;
//                 kprintf(", Addr: %p\n", result);
            }
        }

        for (i = 0; i < MALLOC_TEST_SIZES; i++) {
            for (j = 0; j < MALLOC_TEST_PER_SIZE; j++) {
                ulong result = results[i][j];
                if (result) {
//                     int size = MALLOC_TEST_STEP * (i + 1);
//                     kprintf("\tFreeing size: %d, index: %d, addr: %p", size, j, result);
                    free((void *)result);
//                     kprintf(" Done\n");
                }
            }
        }
    }

    kprintf("Successfully passed the test!\n");
}
