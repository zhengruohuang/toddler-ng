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

#if (defined(LIST_CHECK_OWNER) && LIST_CHECK_OWNER)
    l->head.owner = l;
    l->tail.owner = l;
#endif

    spinlock_init(&l->lock);
}

list_node_t *list_remove(list_t *l, list_node_t *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "List must be locked!\n");
    if (!n || n == &l->head || n == &l->tail) {
        return NULL;
    }

#if (defined(LIST_CHECK_OWNER) && LIST_CHECK_OWNER)
    panic_if(n->owner != l, "List node owner mismatch, list @ %p, owner @ %p\n",
             l, n->owner);
#endif

    list_node_t *prev = n->prev;
    list_node_t *next = n->next;
    prev->next = next;
    next->prev = prev;

    l->count--;
    return n;
}

void list_insert(list_t *l, list_node_t *prev, list_node_t *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "List must be locked!\n");

    list_node_t *next = prev->next;
    next->prev = n;
    prev->next = n;
    n->prev = prev;
    n->next = next;

    l->count++;

#if (defined(LIST_CHECK_OWNER) && LIST_CHECK_OWNER)
    n->owner = l;
#endif
}

void list_insert_sorted(list_t *l, list_node_t *n, list_cmp_t cmp)
{
    panic_if(!spinlock_is_locked(&l->lock), "List must be locked!\n");

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

#if (defined(LIST_CHECK_OWNER) && LIST_CHECK_OWNER)
    n->owner = l;
#endif
}

static void list_merge(list_t *l, list_node_t *n, list_merge_t merger,
                       list_node_t **free1, list_node_t **free2)
{
    // Merge with prev
    if (n->prev != &l->head && !merger(n->prev, n)) {
        if (free1) *free1 = n;
        list_remove(l, n);
        n = n->prev;
    }

    // Merge with next
    if (n->next != &l->tail && !merger(n, n->next)) {
        list_node_t *nn = n->next;
        list_remove(l, nn);
        if (free2) *free2 = nn;
    }
}

void list_insert_merge_sorted(list_t *l, list_node_t *n, list_cmp_t cmp,
                              list_merge_t merger, list_free_t freer)
{
    // Insert
    list_insert_sorted(l, n, cmp);

    // Merge
    list_node_t *free1 = NULL, *free2 = NULL;
    list_merge(l, n, merger, &free1, &free2);

    // Free
    if (free1) freer(free1);
    if (free2) freer(free2);
}

list_node_t *list_front(list_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "List must be locked!\n");
    list_node_t *n = l->head.next;
    return n == &l->tail ? NULL : n;
}

list_node_t *list_back(list_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "List must be locked!\n");
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

void list_display(list_t *l, list_node_display_t d)
{
    panic_if(!spinlock_is_locked(&l->lock), "List must be locked!\n");

    int idx = 0;
    list_foreach(l, n) {
        if (d) {
            d(idx++, n);
        }
    }
}


/*
 * Exclusive
 */
list_node_t *list_remove_exclusive(list_t *l, list_node_t *n)
{
    spinlock_lock_int(&l->lock);
    list_node_t *r = list_remove(l, n);
    spinlock_unlock_int(&l->lock);

    return r;
}

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

void list_insert_merge_sorted_exclusive(list_t *l, list_node_t *n, list_cmp_t cmp,
                                        list_merge_t merger, list_free_t freer)
{
    list_node_t *free1 = NULL, *free2 = NULL;

    spinlock_lock_int(&l->lock);
    list_insert_sorted(l, n, cmp);
    list_merge(l, n, merger, &free1, &free2);
    spinlock_unlock_int(&l->lock);

    if (free1) freer(free1);
    if (free2) freer(free2);
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

void list_display_exclusive(list_t *l, list_node_display_t d)
{
    spinlock_lock_int(&l->lock);
    list_display(l, d);
    spinlock_unlock_int(&l->lock);
}


/*
 * Test
 */
struct test_list_node {
    list_node_t node;
    int num;
};

static list_t list_for_test;
static struct test_list_node nodes_for_test[16];

static int test_list_compare(list_node_t *a, list_node_t *b)
{
    struct test_list_node *ba = list_entry(a, struct test_list_node, node);
    struct test_list_node *bb = list_entry(b, struct test_list_node, node);
    if (ba->num > bb->num) {
        return 1;
    } else if (ba->num < bb->num) {
        return -1;
    } else {
        return 0;
    }
}

static void test_list_node_display(int idx, list_node_t *n)
{
    struct test_list_node *node = list_entry(n, struct test_list_node, node);
    kprintf("#%d: %d\n", idx, node->num);
}

static struct test_list_node *test_list_insert(int num)
{
    static int alloc_idx = 0;
    struct test_list_node *node = &nodes_for_test[alloc_idx++];
    node->num = num;
    list_insert_sorted_exclusive(&list_for_test, &node->node, test_list_compare);
    return node;
}

void test_list()
{
    kprintf("Testing list\n");

    list_init(&list_for_test);
    struct test_list_node *node1 = test_list_insert(1);
    struct test_list_node *node2 = test_list_insert(2);
    struct test_list_node *node3 = test_list_insert(3);
    struct test_list_node *node0 = test_list_insert(0);
    struct test_list_node *node4 = test_list_insert(4);
    struct test_list_node *node32 = test_list_insert(3);
    struct test_list_node *node42 = test_list_insert(4);
    struct test_list_node *node02 = test_list_insert(0);
    list_display_exclusive(&list_for_test, test_list_node_display);

    kprintf("Test\n");
    list_remove_exclusive(&list_for_test, &node3->node);
    list_display_exclusive(&list_for_test, test_list_node_display);

    kprintf("Test\n");
    list_insert_sorted_exclusive(&list_for_test, &node3->node, test_list_compare);
    list_display_exclusive(&list_for_test, test_list_node_display);

    kprintf("Test\n");
    list_access_exclusive(&list_for_test) {
        while (list_for_test.count) {
            list_pop_front(&list_for_test);
        }
    }
    list_display_exclusive(&list_for_test, test_list_node_display);

    kprintf("Test\n");
    list_insert_sorted_exclusive(&list_for_test, &node3->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node4->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node0->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node2->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node1->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node42->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node02->node, test_list_compare);
    list_insert_sorted_exclusive(&list_for_test, &node32->node, test_list_compare);
    list_display_exclusive(&list_for_test, test_list_node_display);

    kprintf("List test done!\n");
}
