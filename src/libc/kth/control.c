#include <atomic.h>
#include <kth.h>
#include <sys.h>
#include <assert.h>


static decl_tls(kth_t *, my_kth_ptr);


static void kth_entry_wrapper(unsigned long param)
{
    kth_t *kth = (void *)param;
    *get_tls(kth_t *, my_kth_ptr) =(void *)kth;

    unsigned long ret = 0;
    if (kth->entry) {
        ret = kth->entry(kth->param);
    }

    kth_exit(ret);
}

int kth_create(kth_t *kth, kth_entry_t entry, unsigned long param)
{
    kth->entry = entry;
    kth->param = param;

    kth->inited = 1;
    kth->exited = 0;
    kth->ret = 0;
    atomic_mb();

    mutex_init_spin(&kth->joined, 0);
    int mutex_err = mutex_trylock(&kth->joined);
    panic_if(mutex_err, "Unable to create thread!");

    kth->tid = syscall_thread_create(kth_entry_wrapper, (unsigned long)kth);
    kth->inited = 0;
    atomic_mb();
    atomic_notify();

    //kprintf_unlocked("tid: %lx\n", kth->tid);

    return 0;
}

kth_t *kth_self()
{
    return *get_tls(kth_t *, my_kth_ptr);
}

int kth_join(kth_t *kth, unsigned long *ret)
{
    while (kth->inited) {
        syscall_thread_yield();
        atomic_mb();
    }

    //atomic_mb();
    while (!kth->exited) {
        mutex_lock(&kth->joined);
        mutex_unlock(&kth->joined);

        //kprintf("still joining!\n");
    }

    if (ret) {
        *ret = kth->ret;
    }

    return 0;
}

void kth_exit(unsigned long ret)
{
    kth_t *self = kth_self();
    self->ret = ret;
    self->tid = 0;
    self->exited = 1;

    atomic_mb();
    atomic_notify();

    mutex_unlock(&self->joined);

    syscall_thread_exit_self(ret);
}

void kth_yield()
{
    syscall_thread_yield();
}
