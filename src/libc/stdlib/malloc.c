#include "common/include/inttypes.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <futex.h>
#include <sys.h>
#include <kth.h>


struct bucket {
    ulong block_size;
    ulong num_blocks_per_chunk;

    ulong num_active_chunks;
    struct chunk *chunk;

    mutex_t lock;
};

struct block {
    struct block *next;
    ulong span;
};

struct chunk {
    struct bucket *bucket;

    ulong num_avail_blocks;
    ulong num_inuse_blocks;
    struct block *block;

    struct chunk *prev;
    struct chunk *next;
};

struct magic {
    struct chunk *chunk;
};


#define CHUNK_SIZE     0x400000ul  // 4 MB
#define MAX_BLOCK_SIZE  (CHUNK_SIZE - sizeof(struct chunk))
#define MIN_BLOCK_SIZE  (sizeof(struct block) > sizeof(struct magic) ? \
                         sizeof(struct block) : sizeof(struct magic))


static int num_buckets = 0;
static struct bucket buckets[33] = { };


/*
 * Init
 */
static struct bucket *new_bucket(size_t block_size, size_t num_blocks_per_chunk)
{
    struct bucket *bucket = &buckets[num_buckets++];
    bucket->block_size = block_size;
    bucket->num_blocks_per_chunk = num_blocks_per_chunk;
    bucket->chunk = NULL;
    mutex_init(&bucket->lock);

    return bucket;
}

void init_malloc()
{
    for (int order = 0; order < 32; order++) {
        size_t block_size = 0x1ul << order;
        if (block_size >= MAX_BLOCK_SIZE) {
            break;
        }

        if (block_size > MIN_BLOCK_SIZE) {
            size_t num_blocks_per_chunk = MAX_BLOCK_SIZE >> order;
            new_bucket(block_size, num_blocks_per_chunk);
        }
    }

    new_bucket(MAX_BLOCK_SIZE, 1);
}


/*
 * Display
 */
void display_malloc()
{
    for (int i = 0; i < num_buckets; i++) {
        struct bucket *bucket = &buckets[i];
        kprintf("Bucket #%d, block: %lu bytes, blocks/chunk: %lu, active chunks: %lu\n",
                i, bucket->block_size, bucket->num_blocks_per_chunk, bucket->num_active_chunks);
    }
}


/*
 * malloc and calloc
 */
static struct bucket *find_bucket(size_t size)
{
    for (int i = 0; i < num_buckets; i++) {
        struct bucket *bucket = &buckets[i];
        if (bucket->block_size >= size) {
            return bucket;
        }
    }

    return NULL;
}

static struct chunk *create_chunk(struct bucket *bucket)
{
    struct chunk *chunk = (void *)syscall_vm_alloc(CHUNK_SIZE, 0);
    //kprintf("new chunk @ %p\n", chunk);

    if (!chunk) {
        return NULL;
    }

    //kprintf("before\n");

    chunk->bucket = bucket;
    chunk->num_avail_blocks = bucket->num_blocks_per_chunk;
    chunk->num_inuse_blocks = 0;

    //kprintf("after\n");

    struct block *block = (void *)chunk + sizeof(struct chunk);
    block->next = NULL;
    block->span = bucket->num_blocks_per_chunk;
    chunk->block = block;

    //kprintf("span: %lx\n", block->span);

    return chunk;
}

static void attach_chunk(struct bucket *bucket, struct chunk *chunk)
{
    chunk->prev = NULL;
    chunk->next = bucket->chunk;
    if (bucket->chunk) {
        bucket->chunk->prev = chunk;
    }
    bucket->num_active_chunks++;
    bucket->chunk = chunk;
}

static void detach_chunk(struct bucket *bucket, struct chunk *chunk)
{
    if (chunk->prev) {
        chunk->prev->next = chunk->next;
    }
    if (chunk->next) {
        chunk->next->prev = chunk->prev;
    }
    if (bucket->chunk == chunk) {
        bucket->chunk = chunk->next;
    }
    bucket->num_active_chunks--;
}

static struct chunk *find_chunk(struct bucket *bucket)
{
    struct chunk *chunk = bucket->chunk;
    if (chunk) {
        //kprintf("chunk found @ %p\n", chunk);
        return chunk;
    }

    chunk = create_chunk(bucket);
    if (chunk) {
        attach_chunk(bucket, chunk);
    }

    return chunk;
}

static struct block *find_block(struct bucket *bucket, struct chunk *chunk)
{
    struct block *block = chunk->block;
    if (block->span == 1) {
        chunk->block = block->next;
    } else {
        struct block *next_block = (void *)block + bucket->block_size;
        next_block->next = block->next;
        next_block->span = block->span - 1;
        chunk->block = next_block;
        //kprintf("span: %lu\n", next_block->span);
    }

    chunk->num_avail_blocks--;
    chunk->num_inuse_blocks++;

    //kprintf("alloc block: %lu, chunk, num_inuse_blocks: %lu, num_avail_blocks: %lu\n",
    //        bucket->block_size,
    //        chunk->num_inuse_blocks, chunk->num_avail_blocks);

    if (!chunk->num_avail_blocks) {
        detach_chunk(bucket, chunk);
    }

    return block;
}

static void *setup_payload(struct chunk *chunk, struct block *block)
{
    struct magic *magic = (void *)block;
    magic->chunk = chunk;
    return (void *)magic + sizeof(struct magic);
}

void *malloc(size_t size)
{
    if (size > MAX_BLOCK_SIZE) {
        return NULL;
    }

    size += sizeof(struct magic);
    struct bucket *bucket = find_bucket(size);
    if (!bucket) {
        return NULL;
    }

    //kprintf("bucket @ %p\n", bucket);

    void *ptr = NULL;

    mutex_exclusive(&bucket->lock) {
        struct chunk *chunk = find_chunk(bucket);
        if (chunk) {
            struct block *block = find_block(bucket, chunk);
            if (block) {
                ptr = setup_payload(chunk, block);
            }
        }
    }

    return ptr;
}

void *calloc(size_t num, size_t size)
{
    size_t total_size = num * size;
    void *ptr = malloc(total_size);

    return ptr;
}


/*
 * free
 */
#define RESERVED_ACTIVE_CHUNKS 0

static void putback_chunk(struct bucket *bucket, struct chunk *chunk)
{
    chunk->prev = NULL;
    chunk->next = bucket->chunk;
    bucket->chunk = chunk;
}

static void putback_block(struct bucket *bucket, struct chunk *chunk, struct block *block)
{
    if (!chunk->num_avail_blocks) {
        putback_chunk(bucket, chunk);
    }

    block->next = chunk->block;
    block->span = 1;
    chunk->block = block;
    chunk->num_inuse_blocks--;
    chunk->num_avail_blocks++;

    //kprintf("free block: %lu, chunk, num_inuse_blocks: %lu, num_avail_blocks: %lu\n",
    //        bucket->block_size,
    //        chunk->num_inuse_blocks, chunk->num_avail_blocks);

    if (!chunk->num_inuse_blocks &&
        bucket->num_active_chunks > RESERVED_ACTIVE_CHUNKS
    ) {
        detach_chunk(bucket, chunk);
        syscall_vm_free((ulong)chunk);
    }
}

void free(void *ptr)
{
    panic_if(!ptr, "Unable to free NULL pointer!\n");

    struct magic *magic = ptr - sizeof(struct magic);
    struct chunk *chunk = magic->chunk;
    struct bucket *bucket = chunk->bucket;

    mutex_exclusive(&bucket->lock) {
        putback_block(bucket, chunk, (void *)magic);
    }
}


/*
 * realloc
 */
void *realloc(void *ptr, size_t new_size)
{
    panic_if(!ptr, "Unable to realloc NULL pointer!\n");

    if (!new_size) {
        free(ptr);
        return NULL;
    }

    struct magic *magic = ptr - sizeof(struct magic);
    struct chunk *chunk = magic->chunk;
    struct bucket *bucket = chunk->bucket;

    size_t real_old_size = bucket->block_size;
    size_t real_new_size = new_size + sizeof(struct magic);
    if (real_new_size <= real_old_size && real_new_size >= real_old_size / 2) {
        return ptr;
    }

    void *new_ptr = malloc(new_size);
    if (new_ptr) {
        size_t old_size = real_old_size - sizeof(struct magic);
        memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
        free(ptr);
    }

    return new_ptr;
}
