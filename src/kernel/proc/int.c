#include "libk/include/string.h"
#include "kernel/include/atomic.h"
#include "kernel/include/proc.h"
#include "kernel/include/kernel.h"
#include "kernel/include/lib.h"


#define MAX_NUM_INT_HANDLERS 32

struct int_handler_ipc {
    int valid;

    ulong seq;
    void *hal_dev;

    struct process *proc;
    ulong entry;
};

struct int_handler_table {
    spinlock_t lock;
    struct int_handler_ipc handlers[MAX_NUM_INT_HANDLERS];
};


/*
 * Interrupt handler table
 */
static struct int_handler_table int_handler_table = { };

static struct int_handler_ipc *alloc_int_handler()
{
    struct int_handler_ipc *h = NULL;

    spinlock_exclusive_int(&int_handler_table.lock) {
        for (ulong seq = 1; seq < MAX_NUM_INT_HANDLERS; seq++) {
            struct int_handler_ipc *handler = &int_handler_table.handlers[seq];
            if (!handler->valid) {
                handler->valid = 1;
                handler->seq = seq;
                h = handler;
                break;
            }
        }
    }

    return h;
}

static void free_int_handler(struct int_handler_ipc *h)
{
    spinlock_exclusive_int(&int_handler_table.lock) {
        memzero(h, sizeof(struct int_handler_ipc));
    }
}


/*
 * Register and unregister user-space interrupt handler
 */
static spinlock_t int_reg_lock = SPINLOCK_INIT;

ulong int_handler_register(struct process *p, ulong fw_id, int fw_pos, ulong entry)
{
    struct int_handler_ipc *h = alloc_int_handler();
    if (!h) {
        return -1ul;
    }

    void *hal_dev = NULL;
    spinlock_exclusive_int(&int_reg_lock) {
        hal_dev = get_hal_exports()->int_register(fw_id, fw_pos, h->seq);
    }

    if (!hal_dev) {
        free_int_handler(h);
        return -1ul;
    }

    h->hal_dev = hal_dev;
    h->proc = p;
    h->entry = entry;
    ref_count_inc(&p->ref_count);

    return h->seq;
}

ulong int_handler_register2(struct process *p, const char *fw_path, int fw_pos, ulong entry)
{
    struct int_handler_ipc *h = alloc_int_handler();
    if (!h) {
        return -1ul;
    }

    void *hal_dev = NULL;
    spinlock_exclusive_int(&int_reg_lock) {
        hal_dev = get_hal_exports()->int_register2(fw_path, fw_pos, h->seq);
    }

    if (!hal_dev) {
        free_int_handler(h);
        return -1ul;
    }

    h->hal_dev = hal_dev;
    h->proc = p;
    h->entry = entry;
    ref_count_inc(&p->ref_count);

    return h->seq;
}

int int_handler_unregister(struct process *p, ulong seq)
{
    panic("Unimplemented!\n");
    return -1;
}


/*
 * Invoke user-space interrupt handler
 */
int int_handler_invoke(ulong seq)
{
    panic_if(seq >= MAX_NUM_INT_HANDLERS,
             "Bad user interrupt seq number: %lu\n", seq);
    if (seq >= MAX_NUM_INT_HANDLERS) {
        return -1;
    }

    int err = -1;
    struct process *proc = NULL;
    ulong entry = 0;

    spinlock_exclusive_int(&int_handler_table.lock) {
        struct int_handler_ipc *handler = &int_handler_table.handlers[seq];
        if (handler->valid) {
            proc = handler->proc;
            entry = handler->entry;

            ref_count_inc(&proc->ref_count);
            err = 0;
        }
    }

    if (proc) {
        create_and_run_thread(tid, t, proc, entry, 0, 0) {
            t->sched_class = SCHED_CLASS_INTERRUPT;
        }
        ref_count_dec(&proc->ref_count);
    }

    return err;
}


/*
 * End-of-interrupt
 */
int int_handler_eoi(ulong seq)
{
    int err = -1;
    struct process *proc = NULL;
    void *hal_dev = NULL;

    spinlock_exclusive_int(&int_handler_table.lock) {
        struct int_handler_ipc *handler = &int_handler_table.handlers[seq];
        if (handler->valid) {
            proc = handler->proc;
            hal_dev = handler->hal_dev;

            ref_count_inc(&proc->ref_count);
            err = 0;
        }
    }

    if (proc) {
        if (hal_dev) {
            get_hal_exports()->int_eoi(hal_dev);
        }
        ref_count_dec(&proc->ref_count);
    }

    return err;
}
