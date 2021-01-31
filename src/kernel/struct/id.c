#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


#define ID_COUNT_PER_RANGE (sizeof(ulong) * sizeof(ulong))


struct id_range {
    ulong key;

    ulong base, count;
    ulong used;

    ulong mask1;
    ulong mask2[sizeof(ulong)];

    list_node_t node_free;
    dict_node_t node_avail;
    dict_node_t node_inuse;
};

typedef struct id_obj {
    ulong base, count;
    ulong used;

    list_t free_ranges;
    dict_t avail_ranges;
    dict_t inuse_ranges;

    spinlock_t lock;
} id_obj_t;


/*
 * ID range struct
 */
static salloc_obj_t id_range_salloc_obj;

static int id_range_list_compare(list_node_t *a, list_node_t *b)
{
    struct id_range *ba = list_entry(a, struct id_range, node_free);
    struct id_range *bb = list_entry(b, struct id_range, node_free);

    if (ba->base > bb->base) {
        return 1;
    } else if (ba->base < bb->base) {
        return -1;
    } else {
        return 0;
    }
}

static int id_range_list_merge(list_node_t *a, list_node_t *b)
{
    struct id_range *ba = list_entry(a, struct id_range, node_free);
    struct id_range *bb = list_entry(b, struct id_range, node_free);

    if (ba->base + ba->count == bb->base) {
        ba->count += bb->count;
        return 0;
    } else {
        return -1;
    }
}

static void id_range_list_free(list_node_t *n)
{
    struct id_range *b = list_entry(n, struct id_range, node_free);
    sfree(b);
}


/*
 * Init
 */
void ialloc_init()
{
    salloc_create_default(&id_range_salloc_obj, "id", sizeof(struct id_range));
}


/*
 * Create and destroy
 */
void ialloc_create(id_obj_t *obj, ulong first, ulong last)
{
    memzero(obj, sizeof(id_obj_t));
    obj->base = first;
    obj->count = last - first + 1;
    obj->used = 0;

    list_init(&obj->free_ranges);
    dict_create_default(&obj->avail_ranges);
    dict_create_default(&obj->inuse_ranges);

    struct id_range *r = salloc(&id_range_salloc_obj);
    r->base = obj->base;
    r->count = obj->count;
    list_push_back_exclusive(&obj->free_ranges, &r->node_free);

    // Lists and Dicts will be protected by ID obj's lock
    spinlock_init(&obj->lock);
}

static void _destroy_free_ranges(list_t *l)
{
    list_node_t *n = list_pop_front_exclusive(l);
    while (n) {
        struct id_range *r = list_entry(n, struct id_range, node_free);
        sfree(r);

        n = list_pop_front_exclusive(l);
    }
}

static void _destroy_inuse_ranges(dict_t *d)
{
    dict_node_t *n = dict_pop_front_exclusive(d);
    while (n) {
        struct id_range *r = list_entry(n, struct id_range, node_inuse);
        sfree(r);

        n = dict_pop_front_exclusive(d);
    }
}

void ialloc_destroy(id_obj_t *obj)
{
    _destroy_free_ranges(&obj->free_ranges);
    _destroy_inuse_ranges(&obj->inuse_ranges);

    dict_destroy(&obj->avail_ranges);
    dict_destroy(&obj->inuse_ranges);

    obj->base = obj->count = 0;
}


/*
 * Alloc
 */
static ulong _id_to_key(ulong id)
{
    return id / ID_COUNT_PER_RANGE;
}

static inline ulong _ialloc_in_range(id_obj_t *obj, struct id_range *r)
{
    int section = ctz(r->mask1);
    int offset = ctz(r->mask2[section]);
    panic_if(!r->mask2[section], "Inconsistent ID range masks!\n");

    r->mask2[section] &= ~(0x1 << offset);
    if (!r->mask2[section]) {
        r->mask1 &= ~(0x1 << section);
    }

    r->used++;
    obj->used++;

    ulong id = r->base + section * sizeof(ulong) + offset;
    return id;
}

static inline long _ialloc_from_avail(id_obj_t *obj)
{
    dict_node_t *n = NULL;
    dict_access_exclusive(&obj->avail_ranges) {
        n = dict_front(&obj->avail_ranges);
    }
    if (!n) {
        return -1;
    }

    struct id_range *r = dict_entry(n, struct id_range, node_avail);
    ulong id = _ialloc_in_range(obj, r);

    if (r->used == r->count) {
        dict_remove_exclusive(&obj->avail_ranges, n);
    }

    return id;
}

static inline int _ialloc_new_avail_range(id_obj_t *obj, struct id_range *new_r)
{
    // TODO
    int new_r_used = 0;

    // Make sure there's indeed no more avail ranges
    dict_node_t *dn = NULL;
    dict_access_exclusive(&obj->avail_ranges) {
        dn = dict_front(&obj->avail_ranges);
    }
    if (!dn) {
        return 0;
    }

    // Get a free range
    list_node_t *ln = NULL;
    list_access_exclusive(&obj->free_ranges) {
        ln = list_front(&obj->free_ranges);
    }
    struct id_range *old_r = list_entry(ln, struct id_range, node_free);

    if (old_r->count <= ID_COUNT_PER_RANGE) {
        // Reuse old range
        new_r = old_r;
        list_pop_front_exclusive(&obj->free_ranges);
    } else {
        new_r_used = 1;
        new_r->base = old_r->base;
        new_r->count = ID_COUNT_PER_RANGE;
        old_r->base += ID_COUNT_PER_RANGE;
        old_r->count -= ID_COUNT_PER_RANGE;
    }
    new_r->used = 0;
    new_r->key = _id_to_key(new_r->base);

    ulong count = new_r->count;
    int section = 0;
    while (count) {
        new_r->mask1 |= 0x1ul << section;
        if (count >= sizeof(ulong)) {
            new_r->mask2[section] = -0x1ul;
            count -= sizeof(ulong);
        } else {
            new_r->mask2[section] = (0x1ul << count) - 0x1ul;
            count = 0;
        }
    }

    dict_insert_exclusive(&obj->avail_ranges, new_r->key, &new_r->node_avail);
    dict_insert_exclusive(&obj->inuse_ranges, new_r->key, &new_r->node_inuse);

    return new_r_used;
}

long ialloc(id_obj_t *obj)
{
    long id = 0;

    // Allocate from avail ranges
    spinlock_exclusive_int(&obj->lock) {
        id = _ialloc_from_avail(obj);
    }
    if (id > -1) {
        return id;
    }

    // Create a new avail range
    int r_used = 0;
    struct id_range *r = salloc(&id_range_salloc_obj);

    spinlock_exclusive_int(&obj->lock) {
        r_used = _ialloc_new_avail_range(obj, r);
        id = _ialloc_from_avail(obj);
    }

    if (!r_used) {
        sfree(r);
    }
    return id;
}


/*
 * Free
 */
static inline struct id_range *_find_range_by_id(id_obj_t *obj, ulong id)
{
    ulong key = _id_to_key(id);
    dict_node_t *n = dict_find(&obj->inuse_ranges, key);
    if (!n) {
        return NULL;
    }

    struct id_range *r = dict_entry(n, struct id_range, node_inuse);
    if (id < r->base || id >= r->base + r->count) {
        return NULL;
    }

    return r;
}

static inline void _ifree_in_range(id_obj_t *obj, ulong id, struct id_range *r)
{
    int section = (id - r->base) / sizeof(ulong);
    int offset = (id - r->base) % sizeof(ulong);
    r->mask2[section] |= 0x1ul << offset;
    r->mask1 |= 0x1ul << section;
    r->used--;
    obj->used--;
}

void ifree(id_obj_t *obj, ulong id)
{
    list_node_t *free1 = NULL, *free2 = NULL;

    spinlock_exclusive_int(&obj->lock) {
        struct id_range *r = _find_range_by_id(obj, id);
        if (!r) {
            break;
        }

        int range_was_full = r->used == r->count;
        _ifree_in_range(obj, id, r);

        // Add back to avail ranges
        if (range_was_full) {
            dict_insert_exclusive(&obj->avail_ranges, r->key, &r->node_avail);
        }

        // Move to free
        if (!r->used) {
            dict_remove_exclusive(&obj->avail_ranges, &r->node_avail);
            dict_remove_exclusive(&obj->inuse_ranges, &r->node_inuse);
            list_insert_merge_sorted_exclusive(&obj->free_ranges, &r->node_free,
                                               id_range_list_compare,
                                               id_range_list_merge,
                                               &free1, &free2);
        }
    }

    if (free1) id_range_list_free(free1);
    if (free2) id_range_list_free(free2);
}
