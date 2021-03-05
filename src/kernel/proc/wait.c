/*
 * Wait queue
 */


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "kernel/include/kernel.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/proc.h"
#include "kernel/include/struct.h"
#include "kernel/include/syscall.h"


/*
 * WaitQ
 */
static list_t wait_queue;

static int wait_queue_compare(list_node_t *a, list_node_t *b)
{
    struct thread *ta = list_entry(a, struct thread, node_wait);
    struct thread *tb = list_entry(b, struct thread, node_wait);
    if (ta->wait_timeout_ms > tb->wait_timeout_ms) {
        return 1;
    } else if (ta->wait_timeout_ms < tb->wait_timeout_ms) {
        return -1;
    } else {
        return 0;
    }
}

list_t *acquire_wait_queue_exclusive()
{
    spinlock_lock_int(&wait_queue.lock);
    return &wait_queue;
}

void release_wait_queue()
{
    spinlock_unlock_int(&wait_queue.lock);
}

ulong get_num_wait_threads()
{
    ulong num = 0;

    list_access_exclusive(&wait_queue) {
        num = wait_queue.count;
    }

    return num;
}


/*
 * Next ready
 */
int is_wait_queue_ready()
{
    u64 cur_ms = hal_get_ms();
    u64 next_ready_ms = -1ull;
    list_access_exclusive(&wait_queue) {
        list_node_t *node = list_front(&wait_queue);
        if (node) {
            struct thread *wait_t = list_entry(node, struct thread, node_wait);
            next_ready_ms = wait_t->wait_timeout_ms;
        }
    };

    return cur_ms >= next_ready_ms ? 1 : 0;
}


/*
 * Wait
 */
int wait_on_object(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong wait_value, ulong timeout_ms)
{
    int do_wait = 0;
    int err = 0;

    spinlock_lock_int(&wait_queue.lock);

    switch (wait_type) {
    case WAIT_ON_TIMEOUT:
        do_wait = 1;
        wait_obj = t->tid;
        break;
    case WAIT_ON_FUTEX: {
        futex_t *futex = NULL;
        if (p == get_kernel_proc()) {
            futex = (void *)wait_obj;
        } else {
            paddr_t paddr = get_hal_exports()->translate(p->page_table, wait_obj);
            if (paddr) {
                futex = hal_cast_paddr_to_kernel_ptr(paddr);
            }
        }

        //paddr_t paddr = get_hal_exports()->translate(p->page_table, wait_obj);
        //if (paddr) {
        //    futex_t *futex = hal_cast_paddr_to_kernel_ptr(paddr);
        if (futex) {
            futex_t futex_val;
            futex_val.value = futex->value;

            int when_ne = wait_value & FUTEX_WHEN_NE ? 1 : 0;
            ulong when_val = wait_value & FUTEX_VALUE_MASK;

            if (futex_val.kernel && (
                (!when_ne && futex->locked == when_val) ||
                ( when_ne && futex->locked != when_val)
            )) {
                do_wait = 1;
            }
        } else {
            err = -1;
        }
        break;
    }
    default:
        err = -1;
        break;
    }

    panic_if(err && do_wait, "Can't do wait on an error!\n");

    if (do_wait) {
        spinlock_exclusive_int(&t->lock) {
            //kprintf("To wait, type: %d, obj: %lx, timeout: %lu\n",
            //        wait_type, wait_obj, timeout_ms);

            t->wait_type = wait_type;
            t->wait_obj = wait_obj;
            t->wait_timeout_ms = timeout_ms ? hal_get_ms() + timeout_ms : -1ull;
        }
    } else {
        spinlock_unlock_int(&wait_queue.lock);
    }

    if (!do_wait) {
        err = -1;
    }
    return err;
}

void sleep_thread(struct thread *t)
{
    //kprintf("Sleep: %x\n", t->tid);
    panic_if(!spinlock_is_locked(&wait_queue.lock), "wait queue must be locked!\n");

    set_thread_state(t, THREAD_STATE_WAIT);
    list_insert_sorted(&wait_queue, &t->node_wait, wait_queue_compare);

    spinlock_unlock_int(&wait_queue.lock);
}


/*
 * Wake
 */
static inline void wakeup_thread(struct thread *t)
{
    //kprintf("Wakeup: %x\n", t->tid);

    set_thread_state(t, THREAD_STATE_NORMAL);
    sched_put(t);
}

static inline void wakeup_thread_as_exit(struct thread *t)
{
    set_thread_state(t, THREAD_STATE_EXIT);
    sched_put(t);
}

static void timeout_wakeup_worker(ulong param)
{
    while (1) {
        list_access_exclusive(&wait_queue) {
            list_node_t *node = list_front(&wait_queue);
            while (node) {
                struct thread *wait_t = list_entry(node, struct thread, node_wait);
                u64 counter_ms = hal_get_ms();
//                 kprintf("wait q: %lu, waiting thread: %lx, cur counter: %llu, timeout counter: %llu\n",
//                        wait_queue.count, wait_t->tid, counter_ms, wait_t->wait_timeout_ms);

                if (wait_t->wait_timeout_ms > counter_ms) {
                    break;
                }

                list_pop_front(&wait_queue);
                wakeup_thread(wait_t);

                node = list_front(&wait_queue);
            }
        }

        syscall_thread_yield();
    }
}

ulong wake_on_object(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong wait_value, ulong max_wakeup_count)
{
    panic_if(!spinlock_is_locked(&wait_queue.lock), "wait queue must be locked!\n");

    int do_wakeup = 0;
    int global_wait = 0;

    switch (wait_type) {
    case WAIT_ON_FUTEX: {
        futex_t *futex = NULL;
        if (p == get_kernel_proc()) {
            futex = (void *)wait_obj;
        } else {
            paddr_t paddr = get_hal_exports()->translate(p->page_table, wait_obj);
            if (paddr) {
                futex = hal_cast_paddr_to_kernel_ptr(paddr);
            }
        }

        if (futex) {
        //paddr_t paddr = get_hal_exports()->translate(p->page_table, wait_obj);
        //if (paddr) {
        //    futex_t *futex = hal_cast_paddr_to_kernel_ptr(paddr);
            futex_t futex_val;
            futex_val.value = futex->value;

            int when_ne = wait_value & FUTEX_WHEN_NE ? 1 : 0;
            ulong when_val = wait_value & FUTEX_VALUE_MASK;

            if (!futex_val.kernel && (
                (!when_ne && futex->locked == when_val) ||
                ( when_ne && futex->locked != when_val)
            )) {
                do_wakeup = 1;
            }

//             if (!futex_val.kernel && futex->locked != wait_value) {
//                 do_wakeup = 1;
//             }
        }
        break;
    }
    default:
        break;
    }

    ulong count = 0;
    if (do_wakeup) {
        list_foreach(&wait_queue, node) {
            struct thread *wait_t = list_entry(node, struct thread, node_wait);
            if (wait_t->wait_type == wait_type && wait_t->wait_obj == wait_obj &&
                (global_wait || p->pid == wait_t->pid)
            ) {
                list_remove_in_foreach(&wait_queue, node);
                wakeup_thread(wait_t);

                count++;
                if (max_wakeup_count && count >= max_wakeup_count) {
                    break;
                }
            }
        }
    }

    return count;
}

ulong wake_on_object_exclusive(struct process *p, struct thread *t, int wait_type, ulong wait_obj, ulong wait_value, ulong max_wakeup_count)
{
    ulong count = 0;
    list_access_exclusive(&wait_queue) {
        count = wake_on_object(p, t, wait_type, wait_obj, wait_value, max_wakeup_count);
    }
    return count;
}


/*
 * Purge
 */
ulong purge_wait_queue(struct process *p)
{
    ulong count = 0;

    list_access_exclusive(&wait_queue) {
        list_foreach(&wait_queue, node) {
            struct thread *wait_t = list_entry(node, struct thread, node_wait);
            if (wait_t->pid == p->pid) {
                list_remove_in_foreach(&wait_queue, node);
                wakeup_thread(wait_t);

                count++;
                //break;
            }
        }
    }

    return count;
}


/*
 * Init wait queue
 */
void init_wait()
{
    kprintf("Initializing wait queue\n");
    list_init(&wait_queue);

    create_and_run_kernel_thread(tid, t, &timeout_wakeup_worker, 0, NULL) {
        t->sched_class = SCHED_CLASS_TIMER;

        kprintf("\tWait timeout wakeup worker created, tid: %lx, thread block base: %p\n",
                tid, t->memory.block_base);
    }
}
