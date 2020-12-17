#ifndef __KERNEL_INCLUDE_SYSCALL_H__
#define __KERNEL_INCLUDE_SYSCALL_H__


#include "common/include/inttypes.h"
#include "kernel/include/proc.h"
#include "hal/include/dispatch.h"
#include "libsys/include/syscall.h"


/*
 * Dispatch
 */
extern void init_dispatch();
extern void dispatch(ulong thread_id, struct kernel_dispatch *kdi);


/*
 * Handler
 */
enum syscall_handler_result {
    SYSCALL_HANDLED_SAVE = 0x1,             // Save context
    SYSCALL_HANDLED_PUT_BACK = 0x2,         // Put current thread back to schedule queue
    SYSCALL_HANDLED_RESUME = 0x4,           // Resume current thread without scheduling a new thread
    SYSCALL_HANDLED_EXIT_THREAD = 0x8,      // Exit current thread
    SYSCALL_HANDLED_SLEEP_THREAD = 0x10,    // Put thread to wait queue

    // Continue executing current thread
    SYSCALL_HANDLED_CONTINUE = SYSCALL_HANDLED_RESUME,

    // Put current thread back to sched queue, and schedule a new thread
    SYSCALL_HANDLED_RESCHED = SYSCALL_HANDLED_PUT_BACK,

    // Don't put current thread to sched queue, and do a new schedule
    SYSCALL_HANDLED_SKIP = 0,

    // Don't put current thread to sched queue, but rather put it in wait queue
    // And do a new schedule
    SYSCALL_HANDLED_SLEEP = SYSCALL_HANDLED_SLEEP_THREAD,

    // Save current thread context as well
    SYSCALL_HANDLED_SAVE_CONTINUE = SYSCALL_HANDLED_SAVE | SYSCALL_HANDLED_RESUME,
    SYSCALL_HANDLED_SAVE_RESCHED = SYSCALL_HANDLED_SAVE | SYSCALL_HANDLED_PUT_BACK,
    SYSCALL_HANDLED_SAVE_SKIP = SYSCALL_HANDLED_SAVE,
    SYSCALL_HANDLED_SAVE_SLEEP = SYSCALL_HANDLED_SAVE | SYSCALL_HANDLED_SLEEP_THREAD,
};

typedef int (*syscall_handler_t)(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_none(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_ping(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_illegal(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_puts(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_interrupt(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_vm_alloc(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_vm_free(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_create(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_yield(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_thread_exit(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_alloc_wait(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_wait(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_wake(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_ipc_handler(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_ipc_request(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_ipc_respond(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_ipc_receive(struct process *p, struct thread *t, struct kernel_dispatch *kdi);


#endif