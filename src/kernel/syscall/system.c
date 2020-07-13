/*
 * System-class syscall handlers
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/proc.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/kernel.h"
#include "kernel/include/syscall.h"


/*
 * None
 */
int syscall_handler_none(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    return SYSCALL_HANDLER_CONTINUE;
}


/*
 * Ping
 */
int syscall_handler_ping(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    kdi->return0 = kdi->param0 + 1;
    kdi->return1 = kdi->param1 + 1;
    return SYSCALL_HANDLER_CONTINUE;
}


/*
 * Yield
 */
int syscall_handler_yield(struct process *p, struct thread *t,
                          struct kernel_dispatch *kdi)
{
    return SYSCALL_HANDLER_SAVE_RESCHED;
}


/*
 * Illegal syscall
 */
int syscall_handler_illegal(struct process *p, struct thread *t,
                            struct kernel_dispatch *kdi)
{
    // TODO: start error handler
    return SYSCALL_HANDLER_SKIP;
}


/*
 * Puts
 */
#define PUTS_BUF_SIZE   64
#define PUTS_MAX_LEN    2048

int syscall_handler_puts(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    char buf[PUTS_BUF_SIZE + 1];
    memzero(buf, PUTS_BUF_SIZE + 1);

    ulong len = kdi->param1;
    if (len > PUTS_MAX_LEN) {
        len = PUTS_MAX_LEN;
    }

    for (ulong vaddr = kdi->param0, copy_len = 0; copy_len < len;
         copy_len += PUTS_BUF_SIZE, vaddr += PUTS_BUF_SIZE
    ) {
        ulong cur_len = len - copy_len;
        if (cur_len> PUTS_BUF_SIZE) {
            cur_len = PUTS_BUF_SIZE;
        }

        paddr_t paddr = get_hal_exports()->translate(p->page_table, vaddr);
        void *kernel_ptr = cast_paddr_to_ptr(paddr);
        memcpy(buf, kernel_ptr, cur_len);
        buf[cur_len] = '\0';

        kprintf("%s", buf);
    }

    return SYSCALL_HANDLER_CONTINUE;
}


/*
 * Interrupt
 */
int syscall_handler_interrupt(struct process *p, struct thread *t,
                              struct kernel_dispatch *kdi)
{
    return SYSCALL_HANDLER_SAVE_RESCHED;
}


/*
 * Thread exit
 */
int syscall_handler_thread_exit(struct process *p, struct thread *t,
                                struct kernel_dispatch *kdi)
{
    // FIXME: need to terminate thread
    return SYSCALL_HANDLER_SKIP;
}

