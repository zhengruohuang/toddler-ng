#ifndef __KERNEL_INCLUDE_STRUCT_H__
#define __KERNEL_INCLUDE_STRUCT_H__


#include "common/include/inttypes.h"
#include "kernel/include/atomic.h"


/*
 * Helpers
 */
#define offsetof(type, member) \
    ((ulong)&((type *)0)->member)

#define container_of(ptr, type, member) \
    ((type *)(((ulong)ptr) - (offsetof(type, member))))


/*
 * List - doubly linked list
 */
#define LIST_CHECK_OWNER 1

typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
#if (defined(LIST_CHECK_OWNER) && LIST_CHECK_OWNER)
    struct list *owner;
#endif
} list_node_t;

typedef struct list {
    volatile ulong count;
    struct list_node head;
    struct list_node tail;
    spinlock_t lock;
} list_t;

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_access_exclusive(l) \
    for (spinlock_t *__lock = &(l)->lock; __lock; __lock = NULL) \
        for (spinlock_lock_int(__lock); __lock; spinlock_unlock_int(__lock), __lock = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define list_foreach(l, node) \
    for (list_node_t *node = (l)->head.next; node != &(l)->tail; node = node->next)

#define list_foreach_exclusive(l, node) \
    list_access_exclusive(l) \
        list_foreach(l, node)

#define list_remove_in_foreach(l, n) \
    do { \
        list_node_t *removed_n = list_remove(l, n); \
        if (removed_n) { \
            n = removed_n->prev; \
        } \
    } while (0)

// entry(a) > entry(b) ? 1 : (entry(a) < entry(b) ? -1 : 0)
typedef int (*list_cmp_t)(list_node_t *a, list_node_t *b);

// 0 == merge
typedef int (*list_merge_t)(list_node_t *a, list_node_t *b);
typedef void (*list_free_t)(list_node_t *n);
typedef void (*list_node_display_t)(int idx, list_node_t *n);

extern void list_init(list_t *l);

extern list_node_t *list_remove(list_t *l, list_node_t *n);
extern void list_insert(list_t *l, list_node_t *prev, list_node_t *n);
extern void list_insert_sorted(list_t *l, list_node_t *n, list_cmp_t cmp);
extern void list_insert_merge_sorted(list_t *l, list_node_t *n, list_cmp_t cmp,
                                     list_merge_t merger,
                                     list_node_t **free1, list_node_t **free2);
extern void list_insert_merge_free_sorted(list_t *l, list_node_t *n, list_cmp_t cmp,
                                          list_merge_t merger, list_free_t freer);

extern void list_push_back(list_t *l, list_node_t *n);
extern void list_push_front(list_t *l, list_node_t *n);

extern list_node_t *list_front(list_t *l);
extern list_node_t *list_back(list_t *l);

extern list_node_t *list_pop_back(list_t *l);
extern list_node_t *list_pop_front(list_t *l);

extern void list_display(list_t *l, list_node_display_t d);

extern list_node_t *list_remove_exclusive(list_t *l, list_node_t *n);
extern void list_insert_exclusive(list_t *l, list_node_t *prev, list_node_t *n);
extern void list_insert_sorted_exclusive(list_t *l, list_node_t *n, list_cmp_t cmp);
extern void list_insert_merge_sorted_exclusive(list_t *l, list_node_t *n, list_cmp_t cmp,
                                               list_merge_t merger,
                                               list_node_t **free1, list_node_t **free2);
extern void list_insert_merge_free_sorted_exclusive(list_t *l, list_node_t *n, list_cmp_t cmp,
                                                    list_merge_t merger, list_free_t freer);

extern void list_push_back_exclusive(list_t *l, list_node_t *n);
extern void list_push_front_exclusive(list_t *l, list_node_t *n);

extern list_node_t *list_pop_back_exclusive(list_t *l);
extern list_node_t *list_pop_front_exclusive(list_t *l);

extern void list_display_exclusive(list_t *l, list_node_display_t d);
extern ulong list_count_exclusive(list_t *l);

extern void test_list();


/*
 * Dictionary -- Hashtable
 */
#define DICT_CHECK_OWNER 1

struct dict;
typedef ulong (*dict_hash_t)(struct dict *d, ulong key);

typedef struct dict_node {
    ulong key;
    list_node_t node;
    list_node_t seq_node;

    struct dict_bucket *bucket;

#if (defined(DICT_CHECK_OWNER) && DICT_CHECK_OWNER)
    struct dict *owner;
#endif
} dict_node_t;

typedef struct dict_bucket {
    list_t nodes;
} dict_bucket_t;

typedef struct dict {
    int rehashable;
    dict_hash_t hash_func;
    ulong hash_func_temp;

    list_t seq_nodes;
    volatile ulong count;

    dict_bucket_t *buckets;
    volatile ulong bucket_count;
    ulong min_bucket_count;

    spinlock_t lock;
    spinlock_t rehash_lock;
} dict_t;

#define dict_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define dict_access_exclusive(d) \
    for (spinlock_t *__lock = &(d)->lock; __lock; __lock = NULL) \
        for (spinlock_lock_int(__lock); __lock; spinlock_unlock_int(__lock), __lock = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define dict_foreach(d, node) \
    for (list_node_t *__ln = (d)->seq_nodes.head.next; __ln != &(d)->seq_nodes.tail; __ln = __ln->next) \
        for (dict_node_t *node = list_entry(__ln, dict_node_t, seq_node); node; node = NULL)

#define dict_foreach_exclusive(d, node) \
    dict_access_exclusive(d) \
        dict_foreach(d, node)

#define dict_remove_in_foreach(d, node) \
    do { \
        if (node) { \
            list_node_t *__removed_ln = &(node)->seq_node; \
            __ln = __removed_ln->prev; \
            dict_remove(d, node); \
        } \
    } while (0)

extern ulong dict_default_hash(dict_t *d, ulong key);

extern void dict_create(dict_t *d, ulong num_buckets, dict_hash_t hash_func, int rehashable);
extern void dict_create_default(dict_t *d);
extern void dict_destroy(dict_t *d);

extern void dict_rehash_exclusive(dict_t *d);

extern dict_node_t *dict_find(dict_t *d, ulong key);
extern int dict_contains(dict_t *d, ulong key);
extern dict_node_t *dict_remove_key(dict_t *d, ulong key);
extern dict_node_t *dict_remove(dict_t *d, dict_node_t *n);
extern void dict_insert(dict_t *d, ulong key, dict_node_t *n);
extern dict_node_t *dict_front(dict_t *d);
extern dict_node_t *dict_pop_front(dict_t *d);

extern int dict_contains_exclusive(dict_t *d, ulong key);
extern dict_node_t *dict_remove_key_exclusive(dict_t *d, ulong key);
extern dict_node_t *dict_remove_exclusive(dict_t *d, dict_node_t *n);
extern void dict_insert_exclusive(dict_t *d, ulong key, dict_node_t *n);
extern dict_node_t *dict_pop_front_exclusive(dict_t *d);


/*
 * ID
 */
#define MAX_NUM_LEVELS 3

typedef struct id_obj {
    ulong base, range;
    ulong alloc_count;

    ulong mask1;
    ulong mask2[sizeof(ulong) * 8];
    ulong *mask3;

    int num_levels;
    ulong *bitmaps[MAX_NUM_LEVELS + 1];

    spinlock_t lock;
} id_obj_t;

extern void id_alloc_create(id_obj_t *obj, ulong first, ulong last);
extern long id_alloc(id_obj_t *obj);
extern void id_free(id_obj_t *obj, ulong id);


#endif
