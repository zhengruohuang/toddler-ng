#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"
#include "libk/include/debug.h"


static decl_tls(kthread_t *, my_kthread_ptr);


static void kthread_entry_wrapper(ulong param)
{
    kthread_t *kth = (void *)param;
    *get_tls(kthread_t *, my_kthread_ptr) =(void *)kth;

    ulong ret = 0;
    if (kth->entry) {
        ret = kth->entry(kth->param);
    }

    kthread_exit(ret);
}

int kthread_create(kthread_t *kth, kthread_entry_t entry, ulong param)
{
    kth->entry = entry;
    kth->param = param;

    kth->init = 1;
    kth->exited = 0;
    kth->ret = 0;
    atomic_mb();

    kth->tid = syscall_thread_create(kthread_entry_wrapper, (ulong)kth);
    kth->init = 0;
    atomic_mb();
    atomic_notify();

    //kprintf_unlocked("tid: %lx\n", kth->tid);

    return 0;
}

kthread_t *kthread_self()
{
    return *get_tls(kthread_t *, my_kthread_ptr);
}

int kthread_join(kthread_t *kth, ulong *ret)
{
    while (kth->init) {
        syscall_thread_yield();
        atomic_mb();
    }

    //atomic_mb();
    while (!kth->exited) {
        syscall_wait_on_thread(kth->tid);
        atomic_pause();
        atomic_mb();

        //kprintf("still joining!\n");
    }

    if (ret) {
        *ret = kth->ret;
    }

    return 0;
}

void kthread_exit(ulong ret)
{
    kthread_t *self = kthread_self();
    self->ret = ret;
    self->tid = 0;
    self->exited = 1;

    atomic_mb();
    atomic_notify();

    syscall_thread_exit_self(ret);
}
