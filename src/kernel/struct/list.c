#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


void list_init(list_t *l)
{
    l->count = 0;
    l->head.prev = NULL;
    l->head.next = &l->tail;
    l->tail.prev = &l->head;
    l->tail.next = NULL;

    spinlock_init(&l->lock);
}

void list_insert(list_t *l, list_node_t *prev, list_node_t *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "list must be locked!\n");

    list_node_t *next = prev->next;
    next->prev = n;
    prev->next = n;
    n->prev = prev;
    n->next = next;

    l->count++;
}

void list_insert_sorted(list_t *l, list_node_t *n, list_cmp_t cmp)
{
    panic_if(!spinlock_is_locked(&l->lock), "list must be locked!\n");

    int inserted = 0;
    for (list_node_t *left = &l->head, *right = l->head.next; left != &l->tail;
         left = right, right = right->next
    ) {
        int cmp_left = left == &l->head ? 1 : cmp(n, left);
        int cmp_right = right == &l->tail ? 1 : cmp(right, n);
        if (cmp_left >= 0 && cmp_right >= 0) {
            right->prev = n;
            left->next = n;
            n->prev = left;
            n->next = right;
            l->count++;
            inserted = 1;
            break;
        }
    }

    panic_if(!inserted, "Unable to insert!\n");
}

list_node_t *list_remove(list_t *l, list_node_t *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "list must be locked!\n");
    if (!n || n == &l->head || n == &l->tail) {
        return NULL;
    }

    list_node_t *prev = n->prev;
    list_node_t *next = n->next;
    prev->next = next;
    next->prev = prev;

    l->count--;
    return n;
}

list_node_t *list_front(list_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "list must be locked!\n");
    list_node_t *n = l->head.next;
    return n == &l->tail ? NULL : n;
}

list_node_t *list_back(list_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "list must be locked!\n");
    list_node_t *n = l->tail.prev;
    return n == &l->head ? NULL : n;
}

void list_push_front(list_t *l, list_node_t *n)
{
    list_insert(l, &l->head, n);
}

void list_push_back(list_t *l, list_node_t *n)
{
    list_insert(l, l->tail.prev, n);
}

list_node_t *list_pop_front(list_t *l)
{
    return list_remove(l, l->head.next);
}

list_node_t *list_pop_back(list_t *l)
{
    return list_remove(l, l->tail.prev);
}


/*
 * Exclusive
 */
void list_insert_exclusive(list_t *l, list_node_t *prev, list_node_t *n)
{
    spinlock_lock_int(&l->lock);
    list_insert(l, prev, n);
    spinlock_unlock_int(&l->lock);
}

void list_insert_sorted_exclusive(list_t *l, list_node_t *n, list_cmp_t cmp)
{
    spinlock_lock_int(&l->lock);
    list_insert_sorted(l, n, cmp);
    spinlock_unlock_int(&l->lock);
}

list_node_t *list_remove_exclusive(list_t *l, list_node_t *n)
{
    spinlock_lock_int(&l->lock);
    list_node_t *r = list_remove(l, n);
    spinlock_unlock_int(&l->lock);

    return r;
}

void list_push_front_exclusive(list_t *l, list_node_t *n)
{
    spinlock_lock_int(&l->lock);
    list_push_front(l, n);
    spinlock_unlock_int(&l->lock);
}

void list_push_back_exclusive(list_t *l, list_node_t *n)
{
    spinlock_lock_int(&l->lock);
    list_push_back(l, n);
    spinlock_unlock_int(&l->lock);
}

list_node_t *list_pop_front_exclusive(list_t *l)
{
    spinlock_lock_int(&l->lock);
    list_node_t *n = list_remove(l, l->head.next);
    spinlock_unlock_int(&l->lock);

    return n;
}

list_node_t *list_pop_back_exclusive(list_t *l)
{
    spinlock_lock_int(&l->lock);
    list_node_t *n = list_remove(l, l->tail.prev);
    spinlock_unlock_int(&l->lock);

    return n;
}
