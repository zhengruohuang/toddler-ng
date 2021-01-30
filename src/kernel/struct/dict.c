#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


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
 * Bucket list compar
 */
static int _dict_list_compare(list_node_t *a, list_node_t *b)
{
    dict_node_t *ba = list_entry(a, dict_node_t, node);
    dict_node_t *bb = list_entry(b, dict_node_t, node);

    if (ba->key > bb->key) {
        return 1;
    } else if (ba->key < bb->key) {
        return -1;
    } else {
        return 0;
    }
}


/*
 * Init
 */
#define DEFAULT_DICT_BUCKET_COUNT 16

void dict_init(dict_t *d, ulong num_buckets, dict_hash_t hash_func, int rehashable)
{
    if (!num_buckets) {
        num_buckets = DEFAULT_DICT_BUCKET_COUNT;
    }

    if (popcount(num_buckets) != 1) {
        num_buckets = align_to_power2_next(num_buckets);
    }

    memzero(d, sizeof(dict_t));

    d->rehashable = rehashable;
    d->hash_func = hash_func;
    d->min_bucket_count = d->bucket_count = num_buckets;
    d->buckets = malloc(sizeof(dict_bucket_t) * num_buckets);

    spinlock_init(&d->lock);
    spinlock_init(&d->rehash_lock);
}

void dict_init_default(dict_t *d)
{
    dict_init(d, DEFAULT_DICT_BUCKET_COUNT, NULL, 1);
}


/*
 * Rehash
 */
static inline int _dict_needs_rehash(dict_t *d)
{
    // Too many nodes
    if (d->count >= d->bucket_count * 2) {
        return 1;
    }

    // Too few nodes
    if (d->count <= d->bucket_count / 8 &&
        d->bucket_count > d->min_bucket_count
    ) {
        return 1;
    }

    return 0;
}

void dict_rehash(dict_t *d)
{
    int need = _dict_needs_rehash(d);
    if (!need) {
        return;
    }

    ulong new_bucket_count = align_to_power2_next(d->count) * 2;
    dict_bucket_t *new_buckets = malloc(sizeof(dict_bucket_t) * new_bucket_count);

    dict_bucket_t *old_buckets = d->buckets;
    free(old_buckets);

    d->buckets = new_buckets;
    d->bucket_count = new_bucket_count;
    d->hash_func_temp = 0;

    // Go through sequential node list
    list_foreach_exclusive(&d->seq_nodes, n) {
        dict_node_t *dn = list_entry(n, dict_node_t, seq_node);

        // FIXME: remove from old bucket, probably unnecessary
        dict_bucket_t *old_b = dn->bucket;
        list_remove_exclusive(&old_b->nodes, &dn->node);

        // Insert to new bucket
        ulong idx = _dict_hash(d, dn->key);
        dict_bucket_t *b = &d->buckets[idx];
        list_insert_sorted_exclusive(&b->nodes, &dn->node, _dict_list_compare);
    }
}

void dict_rehash_exclusive(dict_t *d)
{
    int need = _dict_needs_rehash(d);
    if (!need) {
        return;
    }

    dict_access_exclusive(d) {
        dict_rehash(d);
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

    ulong idx = _dict_hash(d, key);
    dict_bucket_t *b = &d->buckets[idx];

    list_insert_sorted_exclusive(&b->nodes, &n->node, _dict_list_compare);
    list_push_back_exclusive(&d->seq_nodes, &n->seq_node);

    d->count++;
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
