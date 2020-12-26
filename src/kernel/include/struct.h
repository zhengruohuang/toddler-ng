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
#define LIST_CHECK_OWNER    1

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
                                               list_merge_t merger, list_free_t freer);

extern void list_push_back_exclusive(list_t *l, list_node_t *n);
extern void list_push_front_exclusive(list_t *l, list_node_t *n);

extern list_node_t *list_pop_back_exclusive(list_t *l);
extern list_node_t *list_pop_front_exclusive(list_t *l);

extern void list_display_exclusive(list_t *l, list_node_display_t d);

extern void test_list();


/*
 * Hash table
 */
typedef ulong (*hashtable_func_t)(int num_buckets_order, void *key);
typedef int (*hashtable_cmp_t)(void *a, void *b);
                            // key(a) > key(b) ? 1 : (key(a) < key(b) ? -1 : 0)

typedef struct hashtable_node {
    void *key;
    struct hashtable_node *prev;
    struct hashtable_node *next;
} hashtable_node_t;

typedef struct hashtable_bucket {
    ulong count;
    struct hashtable_node head;
    struct hashtable_node tail;
} hashtable_bucket_t;

typedef struct hashtable {
    ulong count;

    int num_buckets_order;
    hashtable_bucket_t *buckets;

    hashtable_func_t hash_func;
    hashtable_cmp_t key_cmp;

    spinlock_t lock;
} hashtable_t;

#define hashtable_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define hashtable_access_exclusive(t) \
    for (spinlock_t *__lock = &(t)->lock; __lock; __lock = NULL) \
        for (spinlock_lock_int(__lock); __lock; spinlock_unlock_int(__lock), __lock = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define hashtable_foreach_bucket(t, b) \
    for (hashtable_bucket_t *b = &(t)->buckets[0]; b < &(t)->buckets[0x1ul << (t)->num_buckets_order]; b++)

#define hashtable_foreach_node(b, n) \
    for (hashtable_node_t *n = (b)->head.next; n != &(b)->tail; n = n->next)

extern void hashtable_init(hashtable_t *t, int num_buckets, hashtable_func_t hash, hashtable_cmp_t cmp);
extern void hashtable_init_default(hashtable_t *t);

extern void hashtable_insert(hashtable_t *t, void *key, hashtable_node_t *n);
extern hashtable_node_t *hashtable_get(hashtable_t *t, void *key);
extern int hashtable_contains(hashtable_t *t, void *key);
extern hashtable_node_t *hashtable_remove(hashtable_t *t, void *key);

extern void hashtable_insert_exlusive(hashtable_t *t, void *key, hashtable_node_t *n);
extern hashtable_node_t *hashtable_get_exclusive(hashtable_t *t, void *key);
extern int hashtable_contains_exclusive(hashtable_t *t, void *key);
extern hashtable_node_t *hashtable_remove_exclusive(hashtable_t *t, void *key);


#endif
