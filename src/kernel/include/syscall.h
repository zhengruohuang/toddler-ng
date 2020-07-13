#ifndef __KERNEL_INCLUDE_SYSCALL_H__
#define __KERNEL_INCLUDE_SYSCALL_H__


#include "common/include/inttypes.h"
#include "common/include/sysenter.h"
#include "common/include/syscall.h"
#include "kernel/include/proc.h"
#include "hal/include/dispatch.h"


/*
 * Invoke
 */
extern void syscall_yield();
extern void syscall_unreachable();
extern void syscall_thread_exit_self();


/*
 * Handler
 */
enum syscall_handler_result {
    SYSCALL_HANDLER_CONTINUE,       // Continue executing current thread
    SYSCALL_HANDLER_RESCHED,        // Put current thread back to sched queue
    SYSCALL_HANDLER_SKIP,           // Don't put current thread to sched queue
    SYSCALL_HANDLER_SAVE_CONTINUE,  // Save current thread state and continue executing current thread
    SYSCALL_HANDLER_SAVE_RESCHED,   // Save current thread state and put it back to sched queue
    SYSCALL_HANDLER_SAVE_SKIP,      // Save current thread state but don't put it to sched queue
};

typedef int (*syscall_handler_t)(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern void init_syscall();
extern int dispatch_syscall(struct process *p, struct thread *t, struct kernel_dispatch *kdi);

extern int syscall_handler_none(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_ping(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_yield(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_illegal(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_puts(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_interrupt(struct process *p, struct thread *t, struct kernel_dispatch *kdi);
extern int syscall_handler_thread_exit(struct process *p, struct thread *t, struct kernel_dispatch *kdi);


#endif
