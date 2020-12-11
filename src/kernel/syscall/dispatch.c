/*
 * Syscall dispatcher
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/proc.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/syscall.h"
#include "kernel/include/kernel.h"


static syscall_handler_t handlers[NUM_SYSCALLS];


void init_dispatch()
{
    memzero(handlers, sizeof(handlers));

    handlers[SYSCALL_PING] = syscall_handler_ping;
    handlers[SYSCALL_PUTS] = syscall_handler_puts;

    handlers[SYSCALL_INTERRUPT] = syscall_handler_interrupt;

    handlers[SYSCALL_VM_ALLOC] = syscall_handler_vm_alloc;
    handlers[SYSCALL_VM_FREE] = syscall_handler_vm_free;

    handlers[SYSCALL_THREAD_CREATE] = syscall_handler_create;
    handlers[SYSCALL_THREAD_YIELD] = syscall_handler_yield;
    handlers[SYSCALL_THREAD_EXIT] = syscall_handler_thread_exit;

    handlers[SYSCALL_EVENT_ALLOC] = syscall_handler_alloc_wait;
    handlers[SYSCALL_EVENT_WAIT] = syscall_handler_wait;
    handlers[SYSCALL_EVENT_WAKE] = syscall_handler_wake;

    handlers[SYSCALL_IPC_HANDLER] = syscall_handler_ipc_handler;
    handlers[SYSCALL_IPC_REQUEST] = syscall_handler_ipc_request;
    handlers[SYSCALL_IPC_RESPOND] = syscall_handler_ipc_respond;
    handlers[SYSCALL_IPC_RECEIVE] = syscall_handler_ipc_receive;
}


void dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    //kprintf("dispatch: %d, MP: %d\n", kdi->num, hal_get_cur_mp_seq());

    struct thread *target_thread = NULL;
    int resume = 0;
    int save_ctxt = 0;
    int put_back = 0;
    int do_exit = 0;
    int do_sleep = 0;

    int valid = 0;
    access_thread(thread_id, t) {
        access_process(t->pid, p) {
            valid = 1;

            // TODO: check thread/process state

            syscall_handler_t handler = syscall_handler_illegal;
            if (kdi->num < NUM_SYSCALLS && handlers[kdi->num]) {
                handler = handlers[kdi->num];
            }

            int handle_type = handler(p, t, kdi);
            save_ctxt = (handle_type & SYSCALL_HANDLED_SAVE) ? 1 : 0;
            put_back = (handle_type & SYSCALL_HANDLED_PUT_BACK) ? 1 : 0;
            resume = (handle_type & SYSCALL_HANDLED_RESUME) ? 1 : 0;
            do_exit = (handle_type & SYSCALL_HANDLED_EXIT_THREAD) ? 1 : 0;
            do_sleep = (handle_type & SYSCALL_HANDLED_SLEEP_THREAD) ? 1 : 0;

            if (do_exit) {
                panic_if(put_back, "Can't put back an exiting thread back to sched queue!\n");
                panic_if(resume, "Can't resume executing an exiting thread!\n");
                target_thread = t;
            } else if (do_sleep) {
                panic_if(put_back, "Can't put back an waiting thread back to sched queue!\n");
                panic_if(resume, "Can't resume executing an waiting thread!\n");
                target_thread = t;
            }

            if (save_ctxt) {
                thread_save_context(t, kdi->regs);
            }

            if (put_back) {
                sched_put(t);
            }

            // The thread will not be put back to sched queue, and it will not be resumed
            // if (!put_back && !resume) {
            if (do_exit) {
                atomic_dec(&t->ref_count.value);
                atomic_mb();
            }
        }
    }

    panic_if(!valid, "Invalid tid: %lx\n", kdi->tid);

    if (do_exit) {
        exit_thread(target_thread);
    } else if (do_sleep) {
        sleep_thread(target_thread);
    }

    if (!resume) {
        sched();
        unreachable();
    }
}
