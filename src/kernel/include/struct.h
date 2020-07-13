#ifndef __KERNEL_INCLUDE_STRUCT_H__
#define __KERNEL_INCLUDE_STRUCT_H__


#include "common/include/inttypes.h"
#include "kernel/include/atomic.h"


/*
 * Singly linked list
 */
typedef struct slist_node {
    struct slist_node *next;
    void *node;
} slist_node_t;

typedef struct slist {
    ulong count;
    struct slist_node *head;
    struct slist_node *tail;

    spinlock_t lock;
} slist_t;

#define slist_foreach(l, node) \
    for (slist_node_t *node = (l)->head; node; node = node->next)

#define slist_access_exclusive(l) \
    for (spinlock_t *__lock = &(l)->lock; __lock; __lock = NULL) \
        for (spinlock_lock_int(__lock); __lock; spinlock_unlock_int(__lock), __lock = NULL) \
            for (int __term = 0; !__term; __term = 1)

#define slist_foreach_exclusive(l, node) \
    for (spinlock_t *__lock = &(l)->lock; __lock; __lock = NULL) \
        for (spinlock_lock_int(__lock); __lock; spinlock_unlock_int(__lock), __lock = NULL) \
            for (slist_node_t *node = (l)->head; node; node = node->next)

extern void init_slist();
extern void slist_create(slist_t *l);

extern void slist_insert(slist_t *l, slist_node_t *prev, void *n);
extern void *slist_remove(slist_t *l, slist_node_t *prev, slist_node_t *s);

extern void *slist_front(slist_t *l);
extern void *slist_back(slist_t *l);

extern void slist_push_back_exclusive(slist_t *l, void *n);
extern void slist_push_front_exclusive(slist_t *l, void *n);

extern void slist_push_back(slist_t *l, void *n);
extern void slist_push_front(slist_t *l, void *n);

extern void *slist_pop_back(slist_t *l);
extern void *slist_pop_front(slist_t *l);



// /*
//  * Hash table
//  */
// typedef ulong (*hashtable_func_t)(ulong key, ulong size);
// typedef int (*hashtable_cmp_t)(ulong cmp_key, ulong node_key);
//
// typedef struct hashtable_node {
//     struct hashtable_node *next;
//     ulong key;
//     void *node;
// } hashtable_node_t;
//
// typedef struct hashtable_bucket {
//     ulong node_count;
//     hashtable_node_t *head;
// } hashtable_bucket_t;
//
// typedef struct hashtable {
//     ulong bucket_count;
//     ulong node_count;
//     hashtable_bucket_t *buckets;
//
//     hashtable_func_t hash_func;
//     hashtable_cmp_t hash_cmp;
//
//     spinlock_t lock;
// } hashtable_t;
//
// extern void init_hashtable();
// extern void hashtable_create(hashtable_t *l, ulong bucket_count, hashtable_func_t hash_func, hashtable_cmp_t hash_cmp);
// extern hashtable_t *hashtable_new(ulong bucket_count, hashtable_func_t hash_func, hashtable_cmp_t hash_cmp);
// extern int hashtable_contains(hashtable_t *l, ulong key);
// extern void *hashtable_obtain(hashtable_t *l, ulong key);
// extern void hashtable_release(hashtable_t *l, ulong key, void *n);
// extern int hashtable_insert(hashtable_t *l, ulong key, void *n);
// extern int hashtable_remove(hashtable_t *l, ulong key);


#endif
