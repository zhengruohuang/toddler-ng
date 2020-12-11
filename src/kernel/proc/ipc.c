/*
 * IPC
 */


#include "common/include/inttypes.h"
#include "common/include/ipc.h"
#include "kernel/include/kernel.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/atomic.h"
#include "kernel/include/proc.h"


struct serial_msg {
    list_node_t node;
    ulong sender_tid;
    int need_reply;
};


/*
 * Init
 */
static salloc_obj_t serial_msg_salloc_obj;

void init_ipc()
{
    kprintf("Initializing IPC\n");

    // Create salloc obj
    salloc_create(&serial_msg_salloc_obj, sizeof(struct serial_msg), 0, 0, NULL, NULL);
}


/*
 * Popup handler
 */
int ipc_reg_popup_handler(struct process *p, struct thread *t, ulong entry)
{
    int err = -1;

    spinlock_exclusive_int(&p->lock) {
        kprintf("reg, is_main: %d, count: %d\n", t->is_main, p->threads.count);
        //if (t->is_main && p->threads.count == 1) {
            err = 0;
            p->popup_msg_handler = entry;
        //}
    }

    return err;
}


/*
 * Request
 */
static void copy_msg(struct thread *dst_t, struct thread *src_t)
{
    msg_t *dst_msg = dst_t->memory.msg.start_paddr_ptr;
    msg_t *src_msg = src_t->memory.msg.start_paddr_ptr;

    dst_msg->size = src_msg->size;

    for (int i = 0; i < src_msg->num_params; i++) {
        dst_msg->params[i] = src_msg->params[i];
    }

    if (src_msg->data_bytes) {
        void *dst_data = (void *)(dst_t->memory.msg.start_paddr_ptr + sizeof(ulong) * src_msg->num_params);
        void *src_data = (void *)(src_t->memory.msg.start_paddr_ptr + sizeof(ulong) * src_msg->num_params);
        memcpy(dst_data, src_data, src_msg->data_bytes);
    }
}

int ipc_request(struct process *p, struct thread *t, ulong dst_pid, ulong opcode, ulong flags, int *wait)
{
    int err = -1;
    *wait = 0;

    access_process(dst_pid, dst_proc) {
        ulong popup_entry = 0;
        if (flags & IPC_SEND_POPUP) {
            spinlock_exclusive_int(&dst_proc->lock) {
                popup_entry = dst_proc->popup_msg_handler;
            }
        }

        // Popup
        if ((flags & IPC_SEND_POPUP) && popup_entry) {
            if (flags & IPC_SEND_WAIT_FOR_REPLY) {
                *wait = 1;
                wait_on_object(p, t, WAIT_ON_MSG_REPLY, t->tid, 0);
            }

            create_and_run_thread(dst_tid, dst_t, dst_proc, popup_entry, opcode, NULL) {
                dst_t->reply_msg_to_tid = t->tid;
                copy_msg(dst_t, t);
                err = 0;
                //kprintf("Popup msg from %lx -> %lx\n", t->tid, dst_tid);
            }
        }

        // FIXME Serial
        else if (flags & IPC_SEND_SERIAL) {
            panic("Serial msg not implemented!\n");
        }
    }

    return err;
}

int ipc_respond(struct process *p, struct thread *t)
{
    int err = -1;
    ulong dst_tid = 0;
    spinlock_exclusive_int(&t->lock) {
        dst_tid = t->reply_msg_to_tid;
        t->reply_msg_to_tid = 0;
    }

    if (dst_tid) {
        access_thread(dst_tid, dst_t) {
            err = 0;
            copy_msg(dst_t, t);
            ulong count = wake_on_object_exclusive(p, t, WAIT_ON_MSG_REPLY, dst_tid, 0);
            panic_if(count != 1, "There must one and only one thread waiting on a msg!\n");
        }
    }

    return err;
}

int ipc_receive(struct process *p, struct thread *t, ulong timeout_ms, ulong *opcode, int *wait)
{
    panic("Serial msg not implemented!\n");
    return -1;
}
