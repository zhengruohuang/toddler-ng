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
static list_t sched_queues[NUM_SCHED_CLASSES];


/*
 * Init
 */
void init_sched()
{
    kprintf("Initializing scheduler\n");

    // Init the queues
    list_init(&sched_queue);

    for (int i = 0; i < NUM_SCHED_CLASSES; i++) {
        list_init(&sched_queues[i]);
    }
}


/*
 * Add to sched queue
 */
void sched_put(struct thread *t)
{
    panic_if(t->state != THREAD_STATE_NORMAL,
             "thread must be in normal state!\n");

    panic_if(t->sched_class >= NUM_SCHED_CLASSES,
             "Thread sched class invalid: %d\n", t->sched_class);

    list_t *sched_q = &sched_queues[t->sched_class];
    list_push_back_exclusive(sched_q, &t->node_sched);
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
static inline struct thread *_pick_from_sched_queue(int class)
{
    list_t *sched_q = &sched_queues[class];
    if (!list_count_exclusive(sched_q)) {
        return NULL;
    }

    struct thread *t = NULL;
    list_access_exclusive(sched_q) {
        list_node_t *n = list_pop_front(sched_q);
        t = n ? list_entry(n, struct thread, node_sched) : NULL;
    }

    return t;
}

static inline struct thread *_pick_from_sched_queue_timer()
{
    if (is_wait_queue_ready()) {
        return _pick_from_sched_queue(SCHED_CLASS_TIMER);
    }
    return NULL;
}

void sched()
{
    //kprintf("Num threads: %lu\n", sched_queue.count);

    // Pick a thread
    struct thread *t = _pick_from_sched_queue(SCHED_CLASS_REAL_TIME);
    if (!t) t = _pick_from_sched_queue_timer();
    if (!t) t = _pick_from_sched_queue(SCHED_CLASS_NORMAL);
    if (!t) t = _pick_from_sched_queue(SCHED_CLASS_IDLE);
    panic_if(!t, "Unable to find a thread\n");

    switch_to_thread(t);
    unreachable();
}
