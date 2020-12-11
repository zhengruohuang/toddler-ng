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


static list_t sched_queue;


/*
 * Init
 */
void init_sched()
{
    kprintf("Initializing scheduler\n");

    // Init the queues
    list_init(&sched_queue);
}


/*
 * Add to sched queue
 */
void sched_put(struct thread *t)
{
    panic_if(t->state != THREAD_STATE_NORMAL,
             "thread must be in normal state!\n");

    list_push_back_exclusive(&sched_queue, &t->node_sched);
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
    //kprintf("To switch, MP seq: %d, page table @ %p, PC @ %p, SP @ %p, R0: %p\n",
    //        hal_get_cur_mp_seq(), t->page_table, t->context.pc, t->context.sp, t->context.r0);

    // Swith to the thread through HAL
    get_hal_exports()->switch_context(t->tid, &t->context, t->page_table,
                        t->user_mode, t->asid,
                        t->memory.block_base + t->memory.tls.start_offset);

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
        list_access_exclusive(&sched_queue) {
            list_node_t *n = list_pop_front(&sched_queue);
            t = n ? list_entry(n, struct thread, node_sched) : NULL;
        }

        if (!t) {
            sched_idle();
        }
    } while (!t);
    assert(t);

    switch_to_thread(t);
    unreachable();
}
