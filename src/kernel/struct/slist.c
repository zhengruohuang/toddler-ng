#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/struct.h"


static int slist_node_salloc_id;


void init_slist()
{
    slist_node_salloc_id = salloc_create(sizeof(slist_node_t), 0, 0, NULL, NULL);
    kprintf("\tSingly linked slist node salloc ID: %d\n", slist_node_salloc_id);
}

void slist_create(slist_t *l)
{
    l->count = 0;
    l->head = NULL;
    l->tail = NULL;

    spinlock_init(&l->lock);
}

void slist_insert(slist_t *l, slist_node_t *prev, void *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");

    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    if (prev) {
        s->next = prev->next;
        prev->next = s;
        if (l->tail == prev) {
            l->tail = s;
        }
    } else {
        s->next = l->head;
        l->head = s;
        if (!l->tail) {
            l->tail = s;
        }
    }

    l->count++;
}

void *slist_remove(slist_t *l, slist_node_t *prev, slist_node_t *s)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");
    if (!s) {
        return NULL;
    }

    if (prev) {
        assert(prev->next == s);

        prev->next = s->next;
        if (l->tail == s) {
            l->tail = prev;
        }
    } else {
        assert(l->head == s);

        if (l->tail == s) {
            l->tail = l->head = NULL;
        } else {
            l->head = s->next;
        }
    }
    return NULL;
}

void *slist_front(slist_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");
    return l->head;
}

void *slist_back(slist_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");
    return l->tail;
}

static inline void push_front_internal(slist_t *l, slist_node_t *s)
{
    s->next = l->head;
    l->head = s;

    if (!l->tail) {
        l->tail = s;
    }

    l->count++;
}

static inline void push_back_internal(slist_t *l, slist_node_t *s)
{
    if (l->tail) {
        l->tail->next = s;
    }
    l->tail = s;

    if (!l->head) {
        l->head = s;
    }

    s->next = NULL;
    l->count++;
}

void slist_push_front(slist_t *l, void *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");

    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    push_front_internal(l, s);
}

void slist_push_back(slist_t *l, void *n)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");

    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    push_back_internal(l, s);
}

void slist_push_front_exclusive(slist_t *l, void *n)
{
    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    spinlock_lock_int(&l->lock);
    push_front_internal(l, s);
    spinlock_unlock_int(&l->lock);
}

void slist_push_back_exclusive(slist_t *l, void *n)
{
    slist_node_t *s = salloc(slist_node_salloc_id);
    assert(s);
    s->node = n;

    spinlock_lock_int(&l->lock);
    push_back_internal(l, s);
    spinlock_unlock_int(&l->lock);
}

void *slist_pop_front(slist_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");

    slist_node_t *s = l->head;
    if (!s) {
        return NULL;
    }

    if (l->tail == s) {
        l->tail = l->head = NULL;
    } else {
        l->head = s->next;
    }

    l->count--;
    void *n = s->node;
    sfree(s);

    return n;
}

void *slist_pop_back(slist_t *l)
{
    panic_if(!spinlock_is_locked(&l->lock), "slist must be locked!\n");

    slist_node_t *s = l->tail;
    if (!s) {
        return NULL;
    }

    if (l->head == s) {
        l->head = l->tail = NULL;
    } else {
        slist_node_t *prev_s = NULL;

        for (slist_node_t *prev = NULL, *cur = l->head; cur; prev = cur, cur = cur->next) {
            if (cur == s) {
                prev_s = prev;
                break;
            }
        }
        assert(prev_s);

        prev_s->next = NULL;
        l->tail = prev_s;
    }

    l->count--;
    void *n = s->node;
    sfree(s);

    return n;
}
