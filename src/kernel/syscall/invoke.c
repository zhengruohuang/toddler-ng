/*
 * Make a syscall from kernel
 */


#include "common/include/inttypes.h"
#include "common/include/errno.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/lib.h"
#include "kernel/include/syscall.h"


void syscall_yield()
{
    sysenter(SYSCALL_YIELD, 0, 0, NULL, NULL);
}

void syscall_thread_exit_self()
{
    sysenter(SYSCALL_THREAD_EXIT, 0, 0, NULL, NULL);
    unreachable();
}
