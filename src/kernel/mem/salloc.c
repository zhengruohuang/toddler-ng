/*
 * Structure Allocator
 */


#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "common/include/mem.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/kernel.h"
#include "kernel/include/atomic.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"


/*
 * Forward decls
 */
struct salloc_bucket;


/*
 * Magic block
 */
struct salloc_magic_block {
    struct salloc_bucket *bucket;
    struct salloc_magic_block *next;
};


/*
 * Bucket
 */
enum salloc_bucket_state {
    bucket_empty,
    bucket_partial,
    bucket_full,
};

struct salloc_bucket {
    // Control fields
    struct salloc_bucket *prev;
    struct salloc_bucket *next;
    enum salloc_bucket_state state;

    // Backlink to the obj struct
    salloc_obj_t *obj;

    // Total number of entries and avail entries
    int entry_count;
    int avail_count;

    // Pointer to the first avail block
    struct salloc_magic_block *block;
};

struct salloc_bucket_list {
    struct salloc_bucket *next;
    int count;
};


/*
 * Bucket manipulation
 */
static struct salloc_bucket *alloc_bucket(salloc_obj_t *obj)
{
    ppfn_t bucket_pfn = palloc_direct_mapped(obj->bucket_page_count);
    paddr_t bucket_paddr = ppfn_to_paddr(bucket_pfn);

    struct salloc_bucket *bucket = cast_paddr_to_ptr(bucket_paddr);
    if (!bucket) {
        return NULL;
    }

    // Initialize the bucket header
    bucket->obj = obj;

    bucket->next = NULL;
    bucket->prev = NULL;
    bucket->state = bucket_empty;

    bucket->entry_count = obj->bucket_block_count;
    bucket->avail_count = obj->bucket_block_count;

    bucket->block = NULL;

    // Initialize all the blocks
    int i;
    for (i = 0; i < bucket->entry_count; i++) {
        struct salloc_magic_block *block = (struct salloc_magic_block *)((ulong)bucket + obj->block_start_offset + i * obj->block_size);

        // Setup block header
        block->bucket = bucket;

        // Push the block to the block list
        block->next = bucket->block;
        bucket->block = block;
    }

    // Done
    return bucket;
}

static void free_bucket(struct salloc_bucket *bucket)
{
    paddr_t paddr = cast_ptr_to_paddr(bucket);
    ppfn_t pfn = paddr_to_ppfn(paddr);
    pfree(pfn);

    //ulong addr = (ulong)bucket;
    //pfree(ADDR_TO_PFN(addr));
}

static void insert_bucket(salloc_obj_t *obj, struct salloc_bucket *bucket)
{
    bucket->next = obj->partial_buckets.next;
    bucket->prev = NULL;

    if (bucket->next) {
        bucket->next->prev = bucket;
    }
    obj->partial_buckets.next = bucket;

    obj->partial_buckets.count++;
}

static void remove_bucket(salloc_obj_t *obj, struct salloc_bucket *bucket)
{
    obj->partial_buckets.count--;

    if (bucket->prev) {
        bucket->prev->next = bucket->next;
    } else {
        obj->partial_buckets.next = bucket->next;
    }

    if (bucket->next) {
        bucket->next->prev = bucket->prev;
    }
}


/*
 * Create a salloc object
 */
void salloc_create(salloc_obj_t *obj, size_t size, size_t align, int count, salloc_callback_t ctor, salloc_callback_t dtor)
{
    // Calculate alignment
    if (!align) {
        align = DATA_ALIGNMENT;
    }

    // Block size
    size_t block_size = size + sizeof(struct salloc_magic_block);
    block_size = align_up_ulong_any(block_size, align);

    // Start offset
    ulong start_offset = sizeof(struct salloc_bucket) + sizeof(struct salloc_magic_block);
    start_offset = align_up_ulong_any(start_offset, align);
    start_offset -= sizeof(struct salloc_magic_block);

    // Block count and page count
    if (!count) {
        count = 32;
    }
    ulong total_size = start_offset + block_size * count;

    // Calculate page count
    ulong page_count = total_size / PAGE_SIZE;
    if (total_size % PAGE_SIZE) {
        page_count++;
    }
    page_count = 0x1 << calc_palloc_order(page_count);

    // Recalculae block count
    int block_count = quo_ulong(page_count * PAGE_SIZE - start_offset, block_size);

    // Initialize the object
    obj->struct_size = size;
    obj->block_size = block_size;

    obj->alignment = align;
    obj->block_start_offset = start_offset;

    obj->constructor = ctor;
    obj->destructor = dtor;

    obj->bucket_page_count = page_count;
    obj->bucket_block_count = block_count;

    obj->partial_buckets.count = 0;
    obj->partial_buckets.next = NULL;

    // Init the lock
    spinlock_init(&obj->lock);

//     // Echo
//     kprintf("\tSalloc object created\n");
//     kprintf("\t\tStruct size: %d\n", obj->struct_size);
//     kprintf("\t\tBlock size: %d\n", obj->block_size);
//     kprintf("\t\tAlignment: %d\n", obj->alignment);
//     kprintf("\t\tStart offset: %d\n", obj->block_start_offset);
//     kprintf("\t\tBucket page count: %d\n", obj->bucket_page_count);
//     kprintf("\t\tBucket block count: %d\n", obj->bucket_block_count);
//     kprintf("\t\tConstructor: %p\n", obj->constructor);
//     kprintf("\t\tDestructor: %p\n", obj->destructor);
}


/*
 * Allocate and deallocate
 */
void *salloc(salloc_obj_t *obj)
{
    //struct salloc_obj *obj = get_obj(obj_id);
    struct salloc_bucket *bucket = NULL;

    // Lock the salloc obj
    spinlock_lock_int(&obj->lock);

    // If there is no partial bucket avail, we need to allocate a new one
    if (!obj->partial_buckets.count) {
        bucket = alloc_bucket(obj);
    } else {
        bucket = obj->partial_buckets.next;
    }

    // If we were not able to obtain a usable bucket, then the fail
    if (!bucket) {
        spinlock_unlock_int(&obj->lock);
        return NULL;
    }

    // Get the first avail block in the bucket;
    struct salloc_magic_block *block = bucket->block;

    // Pop the block from the bucket
    bucket->block = block->next;
    bucket->avail_count--;
    block->next = NULL;

    // Setup the block
    block->bucket = bucket;
//     kprintf("bucket @ %x, obj: %x\n", bucket, obj);

    // Change the bucket state and remove it from the partial list if necessary
    if (!bucket->avail_count) {
        bucket->state = bucket_full;
        remove_bucket(obj, bucket);
    } else if (bucket->avail_count == bucket->entry_count - 1) {
        bucket->state = bucket_partial;
        insert_bucket(obj, bucket);
    }

    // Unlock the salloc obj
    spinlock_unlock_int(&obj->lock);

    // Calculate the final addr of the allocated struct
    void *ptr = (void *)((ulong)block + sizeof(struct salloc_magic_block));

    // Call the constructor
    if (obj->constructor) {
        obj->constructor(ptr);
    }

    return ptr;
}

void sfree(void *ptr)
{
    // Obtain the magic block
    struct salloc_magic_block *block = (struct salloc_magic_block *)((ulong)ptr - sizeof(struct salloc_magic_block));

    // Obtain the bucket and obj
    struct salloc_bucket *bucket = block->bucket;
    salloc_obj_t *obj = bucket->obj;

//     kprintf("bucket @ %x, obj: %x\n", bucket, obj);
//     return;

    // Call the destructor
    if (obj->destructor) {
        obj->destructor(ptr);
    }

    // Lock the salloc obj
    spinlock_lock_int(&obj->lock);

    // Push the block back to the bucket
    block->next = bucket->block;
    bucket->block = block;
    bucket->avail_count++;

    // Change the block state and add it to/remove it from the partil list if necessary
    if (bucket->avail_count == bucket->entry_count) {
        bucket->state = bucket_empty;
        remove_bucket(obj, bucket);
        free_bucket(bucket);
    } else if (1 == bucket->avail_count) {
        bucket->state = bucket_partial;
        insert_bucket(obj, bucket);
    }

    // Unlock the salloc obj
    spinlock_unlock_int(&obj->lock);
}
