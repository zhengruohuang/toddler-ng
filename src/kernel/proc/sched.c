/*
 * Scheduler
 */


#include "common/include/inttypes.h"
// #include "common/include/memory.h"
#include "kernel/include/kernel.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"
#include "kernel/include/atomic.h"
#include "kernel/include/proc.h"


static slist_t sched_queue;


/*
 * Init
 */
void init_sched()
{
    kprintf("Initializing scheduler\n");

    // Init the queues
    slist_create(&sched_queue);
}


/*
 * Add to sched queue
 */
void sched_put(struct thread *t)
{
    panic_if(!spinlock_is_locked(&t->lock), "thread must be locked!\n");
    panic_if(t->state != THREAD_STATE_NORMAL, "thread must be in normal state!\n");

    slist_push_back_exclusive(&sched_queue, t);
}


/*
 * Idling
 */
static void sched_idle()
{
    for (volatile int i = 0; i < 1000; i++);
}


/*
 * Switching
 */
void switch_to_thread(struct thread *t)
{
    kprintf("To switch, MP seq: %d, page table @ %p, PC @ %p, SP @ %p, R0: %p\n",
            hal_get_cur_mp_seq(), t->page_table, t->context.pc, t->context.sp, t->context.r0);

    // Swith to the thread through HAL
    get_hal_exports()->switch_context(t->tid, &t->context, t->page_table,
                        t->user_mode, t->asid,
                        t->memory.block_base + t->memory.tcb_start_offset);

    unreachable();
}


/*
 * The actual scheduler
 */
void sched()
{
    // Pick a thread
    struct thread *t = NULL;
    do {
        slist_access_exclusive(&sched_queue) {
            t = slist_pop_front(&sched_queue);

            if (t) {
                spinlock_lock_int(&t->lock);
                atomic_inc(&t->ref_count.value);
                spinlock_unlock_int(&t->lock);
            }
        }

        if (!t) {
            sched_idle();
        }
    } while (!t);
    assert(t);

    switch_to_thread(t);
    unreachable();
}
