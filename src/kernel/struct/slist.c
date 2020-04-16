#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


static int slist_node_salloc_id;


void init_slist()
{
    slist_node_salloc_id = salloc_create(sizeof(slist_node_t), 0, 0, NULL, NULL);
    kprintf("\tDoubly linked slist node salloc ID: %d\n", slist_node_salloc_id);
}

void slist_create(slist_t *l)
{
    l->count = 0;
    l->head = NULL;
    l->tail = NULL;

    spinlock_init(&l->lock);
}

void slist_push_front(slist_t *l, void *n)
{
    // Allocate a slist node
    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    spinlock_lock_int(&l->lock);

    s->next = l->head;
    l->head = s;
    l->count++;

    spinlock_unlock_int(&l->lock);
}

void slist_push_back(slist_t *l, void *n)
{
    // Allocate a slist node
    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    spinlock_lock_int(&l->lock);

    if (l->tail) l->tail->next = s;
    l->tail = s;
    s->next = NULL;

//     // Push back
//     s->next = NULL;
//     s->prev = l->prev;
//
//     if (l->prev) {
//         l->prev->next = s;
//     }
//     l->prev = s;
//
//     if (!l->next) {
//         l->next = s;
//     }
//
//     l->count++;

    spinlock_unlock_int(&l->lock);
}

// static void inline slist_detach(slist_t *l, slist_node_t *s)
// {
//     if (s->prev) {
//         s->prev->next = s->next;
//     }
//
//     if (s->next) {
//         s->next->prev = s->prev;
//     }
//
//     if (l->prev == s) {
//         l->prev = s->prev;
//     }
//
//     if (l->next == s) {
//         l->next = s->next;
//     }
//
//     l->count--;
// }
//
// void slist_remove(slist_t *l, slist_node_t *s)
// {
//     spinlock_lock_int(&l->lock);
//     slist_detach(l, s);
//     spinlock_unlock_int(&l->lock);
//
//     sfree(s);
// }
//
// void *slist_pop_front(slist_t *l)
// {
//     slist_node_t *s = NULL;
//     void *n = NULL;
//
//     spinlock_lock_int(&l->lock);
//
//     if (l->count) {
//         assert(l->next);
//
//         s = l->next;
//         slist_detach(l, s);
//     }
//
//     spinlock_unlock_int(&l->lock);
//
//     n = s->node;
//     sfree(s);
//
//     return n;
// }
