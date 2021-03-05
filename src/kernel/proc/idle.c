/*
 * Idling thread
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/kernel.h"
#include "kernel/include/proc.h"


static void idle_worker(ulong param)
{
    //kprintf("Idling thread #%lu started\n", param);

    while (1) {
        //kprintf("Idle #%d\n", param);
        hal_idle();
    }
}


void init_idle()
{
    int num_cpus = hal_get_num_cpus();
    for (int i = 0; i < num_cpus; i++) {
        create_and_run_kernel_thread(tid, t, &idle_worker, i, NULL) {
            t->sched_class = SCHED_CLASS_IDLE;

            kprintf("\tIdling thread #%d created, thread ID: %p\n", i,
                    tid, t->memory.block_base);
        }
    }
}
