#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


/*
 * Hash
 */
ulong dict_default_hash(dict_t *d, ulong key)
{
    ulong shift = d->hash_func_temp;
    if (!shift) {
        shift = d->hash_func_temp = ctz(d->bucket_count);
    }

    ulong mask = d->bucket_count - 1;
    ulong h = (key & mask) ^ ((key >> shift) & mask);

    return h;
}

static inline ulong _dict_hash(dict_t *d, ulong key)
{
    if (d->hash_func) {
        return d->hash_func(d, key);
    }

    return dict_default_hash(d, key);
}


/*
 * Bucket list compare
 */
static int _dict_list_compare(list_node_t *a, list_node_t *b)
{
    dict_node_t *ba = list_entry(a, dict_node_t, node);
    dict_node_t *bb = list_entry(b, dict_node_t, node);

#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
    panic_if(ba->owner != bb->owner,
             "Dict node owner mismatch during traversal, a @ %p, b @ %p\n",
             ba->owner, bb->owner);
#endif

    if (ba->key > bb->key) {
        return 1;
    } else if (ba->key < bb->key) {
        return -1;
    } else {
        return 0;
    }
}


/*
 * Bucket alloc
 */
static dict_bucket_t *_dict_alloc_and_init_buckets(ulong count)
{
    ulong size = sizeof(dict_bucket_t) * count;
    ulong num_pages = get_vpage_count(size);

    dict_bucket_t *b = palloc_ptr(num_pages);
    for (ulong i = 0; i < count; i++) {
        list_init(&b[i].nodes);
    }

    return b;
}


/*
 * Init
 */
#define MIN_DICT_BUCKET_COUNT (PAGE_SIZE / sizeof(dict_bucket_t))

void dict_create(dict_t *d, ulong num_buckets, dict_hash_t hash_func, int rehashable)
{
    if (num_buckets < MIN_DICT_BUCKET_COUNT) {
        num_buckets = MIN_DICT_BUCKET_COUNT;
    }

    if (popcount(num_buckets) != 1) {
        num_buckets = align_to_power2_next(num_buckets);
    }

    memzero(d, sizeof(dict_t));

    d->rehashable = rehashable;
    d->hash_func = hash_func;
    d->min_bucket_count = d->bucket_count = num_buckets;
    d->buckets = _dict_alloc_and_init_buckets(num_buckets);

    list_init(&d->seq_nodes);

    spinlock_init(&d->lock);
    spinlock_init(&d->rehash_lock);
}

void dict_create_default(dict_t *d)
{
    dict_create(d, 0, NULL, 1);
}

void dict_destroy(dict_t *d)
{
    if (d && d->buckets) {
        pfree_ptr(d->buckets);
    }
}


/*
 * Rehash
 */
static inline int _dict_needs_rehash(dict_t *d)
{
    if (!d->rehashable) {
        return 0;
    }

    // Too many nodes
    if (d->count >= d->bucket_count * 2) {
        return 1;
    }

    // Too few nodes
    if (d->count <= d->bucket_count / 16 &&
        d->bucket_count > d->min_bucket_count
    ) {
        return 1;
    }

    return 0;
}

static inline dict_bucket_t *_dict_rehash(dict_t *d, ulong new_bucket_count,
                                          dict_bucket_t *new_buckets)
{
    int need = _dict_needs_rehash(d);
    if (!need) {
        return NULL;
    }

    dict_bucket_t *old_buckets = d->buckets;

    d->buckets = new_buckets;
    d->bucket_count = new_bucket_count;
    d->hash_func_temp = 0;

    // Go through sequential node list
    list_foreach_exclusive(&d->seq_nodes, n) {
        dict_node_t *dn = list_entry(n, dict_node_t, seq_node);

#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
        panic_if(dn->owner != d,
                 "Dict node owner mismatch, dict @ %p, owner @ %p\n",
                 d, dn->owner);
#endif

        // FIXME: remove from old bucket, probably unnecessary
        dict_bucket_t *old_b = dn->bucket;
        list_remove_exclusive(&old_b->nodes, &dn->node);

        // Insert to new bucket
        ulong idx = _dict_hash(d, dn->key);
        dict_bucket_t *b = &d->buckets[idx];
        list_insert_sorted_exclusive(&b->nodes, &dn->node, _dict_list_compare);
    }

    return old_buckets;
}

void dict_rehash_exclusive(dict_t *d)
{
    int need = _dict_needs_rehash(d);
    if (!need) {
        return;
    }

    // Grab rehash lock
    int err = spinlock_trylock_int(&d->rehash_lock);
    if (err) {
        return;
    }

    // Allocate new buckets
    ulong new_bucket_count = align_to_power2_next(d->count) * 2;
    if (new_bucket_count < MIN_DICT_BUCKET_COUNT) {
        new_bucket_count = MIN_DICT_BUCKET_COUNT;
    }
    dict_bucket_t *new_buckets = _dict_alloc_and_init_buckets(new_bucket_count);

    // Now rehash all elements
    dict_bucket_t *old_buckets = NULL;
    dict_access_exclusive(d) {
        old_buckets = _dict_rehash(d, new_bucket_count, new_buckets);
    }

    // Free rehash lock
    spinlock_unlock_int(&d->rehash_lock);

    // Free old or unused new buckets
    if (old_buckets) {
        pfree_ptr(old_buckets);
    } else {
        pfree_ptr(new_buckets);
    }
}


/*
 * Operations
 */
dict_node_t *dict_find(dict_t *d, ulong key)
{
    dict_node_t *keynode = NULL;

    ulong idx = _dict_hash(d, key);
    dict_bucket_t *b = &d->buckets[idx];

    list_foreach_exclusive(&b->nodes, n) {
        dict_node_t *dn = list_entry(n, dict_node_t, node);
        if (dn->key == key) {
            keynode = dn;
            break;
        } else if (dn->key > key) {
            break;
        }
    }

#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
    if (keynode) {
        panic_if(keynode->owner != d,
                 "Dict node owner mismatch, dict @ %p, owner @ %p\n",
                 d, keynode->owner);
    }
#endif

    return keynode;
}

int dict_contains(dict_t *d, ulong key)
{
    dict_node_t *keynode = dict_find(d, key);
    int found = keynode ? 1 : 0;
    return found;
}

dict_node_t *dict_remove(dict_t *d, dict_node_t *n)
{
#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
    panic_if(n->owner != d,
             "Dict node owner mismatch, dict @ %p, owner @ %p\n",
             d, n->owner);
#endif

    dict_bucket_t *b = n->bucket;
    list_remove_exclusive(&b->nodes, &n->node);
    list_remove_exclusive(&d->seq_nodes, &n->seq_node);

    d->count--;
    return n;
}

dict_node_t *dict_remove_key(dict_t *d, ulong key)
{
    dict_node_t *keynode = dict_find(d, key);
    if (keynode) {
        dict_remove(d, keynode);
    }

    return keynode;
}

void dict_insert(dict_t *d, ulong key, dict_node_t *n)
{
    n->key = key;
#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
    n->owner = d;
#endif

    ulong idx = _dict_hash(d, key);
    dict_bucket_t *b = &d->buckets[idx];

    list_insert_sorted_exclusive(&b->nodes, &n->node, _dict_list_compare);
    list_push_back_exclusive(&d->seq_nodes, &n->seq_node);

    d->count++;
}

dict_node_t *dict_front(dict_t *d)
{
    list_node_t *ln = list_front(&d->seq_nodes);
    if (!ln) {
        return NULL;
    }

    dict_node_t *dn = list_entry(ln, dict_node_t, seq_node);
#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
    if (dn) {
        panic_if(dn->owner != d,
                 "Dict node owner mismatch, dict @ %p, owner @ %p\n",
                 d, dn->owner);
    }
#endif

    return dn;
}

dict_node_t *dict_pop_front(dict_t *d)
{
    dict_node_t *dn = dict_front(d);
    if (dn) {
        dict_remove(d, dn);
    }

    return dn;
}


/*
 * Exclusive
 */
int dict_contains_exclusive(dict_t *d, ulong key)
{
    int contains = -1;

    dict_access_exclusive(d) {
        contains = dict_contains(d, key);
    }

    return contains;
}

dict_node_t *dict_remove_key_exclusive(dict_t *d, ulong key)
{
    dict_node_t *node = NULL;

    dict_access_exclusive(d) {
        node = dict_remove_key(d, key);
    }

    return node;
}

dict_node_t *dict_remove_exclusive(dict_t *d, dict_node_t *n)
{
    dict_node_t *node = NULL;

    dict_access_exclusive(d) {
        node = dict_remove(d, n);
    }

    return node;
}

void dict_insert_exclusive(dict_t *d, ulong key, dict_node_t *n)
{
    dict_access_exclusive(d) {
        dict_insert(d, key, n);
    }
}

dict_node_t *dict_pop_front_exclusive(dict_t *d)
{
    dict_node_t *n = NULL;

    dict_access_exclusive(d) {
        n = dict_pop_front(d);
    }

    return n;
}
