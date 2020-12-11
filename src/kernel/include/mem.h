#ifndef __KERNEL_INCLUDE_MEM__
#define __KERNEL_INCLUDE_MEM__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "libk/include/memmap.h"
#include "libk/include/mem.h"
#include "kernel/include/atomic.h"


/*
 * PFN database
 */
struct pfndb_entry {
    union {
        u16 flags;

        struct {
            u16 usable      : 1;
            u16 mapped      : 1;
            u16 tags        : 10;
            u16 inuse       : 1;
            u16 zeroed      : 1;
            u16 kernel      : 1;
            u16 swappable   : 1;
        };
    };
} packed2_struct;

extern struct pfndb_entry *get_pfn_entry_by_pfn(ppfn_t pfn);
extern struct pfndb_entry *get_pfn_entry_by_paddr(paddr_t paddr);

extern psize_t get_mem_range(paddr_t *start, paddr_t *end);
extern ppfn_t get_pfn_range(ppfn_t *start, ppfn_t *end);

extern void reserve_pfndb();
extern void init_pfndb();


/*
 * Page frame allocator
 */
extern ppfn_t palloc_direct_mapped(int count);
extern ppfn_t palloc(int count);
extern int pfree(ppfn_t pfn);

extern void *palloc_ptr(int count);
extern int pfree_ptr(void *ptr);

extern void reserve_palloc();
extern void init_palloc();

extern int calc_palloc_order(int count);
extern void buddy_print();
extern void test_palloc();


/*
 * Struct allocator
 */
typedef void (*salloc_callback_t)(void *entry);

struct salloc_bucket;

typedef struct salloc_obj {
    // Sizes
    size_t struct_size;
    size_t block_size;

    // Alignment
    size_t alignment;
    ulong block_start_offset;

    // Constructor/Destructor
    salloc_callback_t constructor;
    salloc_callback_t destructor;

    // Bucket info
    int bucket_page_count;
    int bucket_block_count;

    // Buckets
    //  Note that we only have partial list since empty buckets are freed
    //  immediately, and full buckets are dangling, they will be put back to
    //  the partial list when they become partial
    struct {
        struct salloc_bucket *next;
        ulong count;
    } partial_buckets;

    // The spin lock that protects the entire salloc obj
    // not very efficient, but it does the job
    spinlock_t lock;
} salloc_obj_t;

#define SALLOC_OBJ_INIT { \
        .struct_size = 0, \
        .block_size = 0, \
        .lock = SPINLOCK_INIT \
    }

extern void salloc_create(salloc_obj_t *obj, size_t size, size_t align, int count, salloc_callback_t ctor, salloc_callback_t dtor);
extern void *salloc(salloc_obj_t *obj);
extern void sfree(void *ptr);


/*
 * General memory allocator
 */
extern void init_malloc();
extern void *malloc(size_t size);
extern void *calloc(ulong count, size_t size);
extern void free(void *ptr);
extern void test_malloc();


#endif
