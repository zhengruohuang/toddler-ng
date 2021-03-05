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
    handlers[SYSCALL_INT_HANDLER] = syscall_handler_int_register;
    handlers[SYSCALL_INT_EOI] = syscall_handler_int_eoi;

    handlers[SYSCALL_FAULT_PAGE] = syscall_handler_fault_page;

    handlers[SYSCALL_PROCESS_CREATE] = syscall_handler_process_create;
    handlers[SYSCALL_PROCESS_EXIT] = syscall_handler_process_exit;
    handlers[SYSCALL_PROCESS_RECYCLE] = syscall_handler_process_recycle;

    handlers[SYSCALL_VM_ALLOC] = syscall_handler_vm_alloc;
    handlers[SYSCALL_VM_MAP] = syscall_handler_vm_map;
    handlers[SYSCALL_VM_MAP_CROSS] = syscall_handler_vm_map_cross;
    handlers[SYSCALL_VM_FREE] = syscall_handler_vm_free;

    handlers[SYSCALL_THREAD_CREATE] = syscall_handler_thread_create;
    handlers[SYSCALL_THREAD_CREATE_CROSS] = syscall_handler_thread_create_cross;
    handlers[SYSCALL_THREAD_YIELD] = syscall_handler_thread_yield;
    handlers[SYSCALL_THREAD_EXIT] = syscall_handler_thread_exit;

    handlers[SYSCALL_EVENT_WAIT] = syscall_handler_wait;
    handlers[SYSCALL_EVENT_WAKE] = syscall_handler_wake;

    handlers[SYSCALL_IPC_HANDLER] = syscall_handler_ipc_register;
    handlers[SYSCALL_IPC_REQUEST] = syscall_handler_ipc_request;
    handlers[SYSCALL_IPC_RESPOND] = syscall_handler_ipc_respond;
    handlers[SYSCALL_IPC_RECEIVE] = syscall_handler_ipc_receive;

    handlers[SYSCALL_STATS_KERNEL] = syscall_handler_stats_kernel;
}


void dispatch(ulong thread_id, struct kernel_dispatch *kdi)
{
    //kprintf("dispatch: %d, MP: %d\n", kdi->num, hal_get_cur_mp_seq());

    service_tlb_shootdown_requests();

    struct thread *target_thread = NULL;
    int resume = 0;
    int save_ctxt = 0;
    int put_back = 0;
    int do_exit = 0;
    int do_sleep = 0;
    int do_ipc = 0;

    int valid = 0;
    access_thread(thread_id, t) {
        access_process(t->pid, p) {
            valid = 1;
            target_thread = t;

            // The thread is waiting to exit
            if (t->state == THREAD_STATE_EXIT) {
                do_exit = 1;
            }

            // The process is waiting to exit
            else if (p->state == PROCESS_STATE_STOPPED) {
                set_thread_state(t, THREAD_STATE_EXIT);
                do_exit = 1;
            }

            // Handle the syscall
            else {
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
                do_ipc = (handle_type & SYSCALL_HANDLED_SLEEP_IPC) ? 1 : 0;

                if (save_ctxt) {
                    thread_save_context(t, kdi->regs);
                }
            }
        }
    }

    panic_if(!valid, "Invalid tid: %lx\n", kdi->tid);

    if (do_exit) {
        panic_if(put_back, "Can't put an exiting thread to sched queue!\n");
        panic_if(resume, "Can't resume an exiting thread!\n");
        ref_count_dec(&target_thread->ref_count);
        exit_thread(target_thread);
    } else if (do_sleep) {
        panic_if(put_back, "Can't put an waiting thread to sched queue!\n");
        panic_if(resume, "Can't resume an waiting thread!\n");
        sleep_thread(target_thread);
    } else if (do_ipc) {
        panic_if(put_back, "Can't put an IPC thread to sched queue!\n");
        panic_if(resume, "Can't resume an IPC thread!\n");
        sleep_ipc_thread(target_thread);
    } else if (put_back) {
        sched_put(target_thread);
    }

    if (!resume) {
        sched();
        unreachable();
    }
}
