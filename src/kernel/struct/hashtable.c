#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


/*
 * Default hash
 */
#define MAX_NUM_BUCKETS_ORDER   16

static ulong default_hash_func(int num_buckets_order, void *key)
{
    ulong mask = 0x1ul << num_buckets_order;
    ulong hash = 0;
    for (ulong ukey = (ulong)key; ukey; ukey >>= num_buckets_order) {
        hash ^= ukey & mask;
    }

    panic_if(hash >= (0x1ul << num_buckets_order), "Bad hash!\n");
    return hash;
}

static int default_key_cmp(void *a, void *b)
{
    ulong ua = (ulong)a;
    ulong ub = (ulong)b;
    return ua > ub ? 1 : (ua < ub ? -1 : 0);
}


/*
 * Init
 */
void hashtable_init(hashtable_t *t, int num_buckets_order, hashtable_func_t hash, hashtable_cmp_t cmp)
{
    panic_if(num_buckets_order > MAX_NUM_BUCKETS_ORDER, "Num buckets order too big!\n");

    ulong bucket_mem_size = (0x1ul << num_buckets_order) * sizeof(void *);
    bucket_mem_size = align_up_vaddr(bucket_mem_size, PAGE_SIZE);

    ulong bucket_num_pages = get_vpage_count(bucket_mem_size);
    ulong num_buckets = bucket_mem_size / sizeof(void *);
    panic_if(popcount(num_buckets) != 1, "Bad num_buckets!\n");

    num_buckets_order = ctz(num_buckets);
    if (num_buckets_order > MAX_NUM_BUCKETS_ORDER) {
        num_buckets_order = MAX_NUM_BUCKETS_ORDER;
    }

    t->count = 0;
    t->num_buckets_order = num_buckets_order;
    t->buckets = palloc_ptr(bucket_num_pages);
    t->hash_func = hash ? hash : default_hash_func;
    t->key_cmp = cmp ? cmp : default_key_cmp;
    panic_if(!t->buckets, "Unable to allocate buckets mem!\n");

    for (int i = 0; i < num_buckets; i++) {
        hashtable_bucket_t *b = &t->buckets[i];
        b->count = 0;
        b->head.key = NULL;
        b->head.prev = NULL;
        b->head.next = &b->tail;
        b->tail.key = NULL;
        b->tail.prev = &b->head;
        b->tail.next = NULL;
    }

    spinlock_init(&t->lock);
}

void hashtable_init_default(hashtable_t *t)
{
    hashtable_init(t, 0, NULL, NULL);
}


/*
 * Operations
 */
static hashtable_bucket_t *get_bucket(hashtable_t *t, void *key)
{
    ulong hash = t->hash_func(t->num_buckets_order, key);
    return &t->buckets[hash];
}

static hashtable_node_t *get_node_in_bucket(hashtable_t *t, hashtable_bucket_t *b, void *key)
{
    for (hashtable_node_t *n = b->head.next; n != &b->tail; n = n->next) {
        int c = t->key_cmp(key, n->key);
        if (!c) {
            return n;
        } else if (c < 0) {
            break;
        }
    }

    return NULL;
}

void hashtable_insert(hashtable_t *t, void *key, hashtable_node_t *n)
{
    panic_if(!spinlock_is_locked(&t->lock), "hashtable must be locked!\n");

    int inserted = 0;
    hashtable_bucket_t *b = get_bucket(t, key);

    for (hashtable_node_t *left = &b->head, *right = b->head.next; left != &b->tail;
         left = right, right = right->next
    ) {
        int cmp_left = left == &b->head ? 1 : t->key_cmp(n, left);
        int cmp_right = right == &b->tail ? 1 : t->key_cmp(right, n);
        panic_if(!cmp_left || !cmp_right, "Dupplicated key!\n");

        if (cmp_left > 0 && cmp_right > 0) {
            right->prev = n;
            left->next = n;
            n->prev = left;
            n->next = right;
            b->count++;
            t->count++;
            inserted = 1;
            break;
        }
    }

    panic_if(!inserted, "Unable to insert, bad cmp func!\n");
}

hashtable_node_t *hashtable_get(hashtable_t *t, void *key)
{
    panic_if(!spinlock_is_locked(&t->lock), "hashtable must be locked!\n");

    hashtable_bucket_t *b = get_bucket(t, key);
    hashtable_node_t *n = get_node_in_bucket(t, b, key);
    return n;
}

int hashtable_contains(hashtable_t *t, void *key)
{
    return hashtable_get(t, key) ? 1 : 0;
}

hashtable_node_t *hashtable_remove(hashtable_t *t, void *key)
{
    panic_if(!spinlock_is_locked(&t->lock), "hashtable must be locked!\n");

    hashtable_bucket_t *b = get_bucket(t, key);
    hashtable_node_t *n = get_node_in_bucket(t, b, key);
    if (!n) {
        return NULL;
    }

    hashtable_node_t *prev = n->prev;
    hashtable_node_t *next = n->next;
    prev->next = next;
    next->prev = prev;

    b->count--;
    t->count--;
    return n;
}


/*
 * Exclusive
 */
void hashtable_insert_exlusive(hashtable_t *t, void *key, hashtable_node_t *n)
{
    hashtable_access_exclusive(t) {
        hashtable_insert(t, key, n);
    }
}

hashtable_node_t *hashtable_get_exclusive(hashtable_t *t, void *key)
{
    hashtable_node_t *n = NULL;
    hashtable_access_exclusive(t) {
        n = hashtable_get(t, key);
    }
    return n;
}

int hashtable_contains_exclusive(hashtable_t *t, void *key)
{
    int contains = 0;
    hashtable_access_exclusive(t) {
        contains = hashtable_contains(t, key);
    }
    return contains;
}

hashtable_node_t *hashtable_remove_exclusive(hashtable_t *t, void *key)
{
    hashtable_node_t *n = NULL;
    hashtable_access_exclusive(t) {
        n = hashtable_remove(t, key);
    }
    return n;
}
