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
#include "libsys/include/ipc.h"


/*
 * WaitQ
 */
enum ipc_wait_type {
    IPC_WAIT_FOR_RESP,          // popup/serial sender waiting for response
    IPC_WAIT_FOR_SERIAL_RECV,   // serial sender waiting for receiver
    IPC_WAIT_FOR_SERIAL_REQ,    // serial receiver waiting for req
};

static list_t ipc_wait_queue;

static inline void wakeup_thread(struct thread *t)
{
    //kprintf("Wakeup: %x\n", t->tid);

    set_thread_state(t, THREAD_STATE_NORMAL);
    sched_put(t);
}

void sleep_ipc_thread(struct thread *t)
{
    panic_if(!spinlock_is_locked(&ipc_wait_queue.lock),
             "wait queue must be locked!\n");

    set_thread_state(t, THREAD_STATE_WAIT);
    list_push_front(&ipc_wait_queue, &t->node_wait);

    spinlock_unlock_int(&ipc_wait_queue.lock);
}

ulong get_num_ipc_threads()
{
    ulong num = 0;

    list_access_exclusive(&ipc_wait_queue) {
        num = ipc_wait_queue.count;
    }

    return num;
}


/*
 * Init
 */
void init_ipc()
{
    kprintf("Initializing IPC\n");

    list_init(&ipc_wait_queue);
}


/*
 * Popup handler
 */
int ipc_reg_popup_handler(struct process *p, struct thread *t, ulong entry)
{
    int err = -1;

    spinlock_exclusive_int(&p->lock) {
        //kprintf("reg, is_main: %d, count: %d\n", t->is_main, p->threads.count);
        //if (t->is_main && p->threads.count == 1) {
            err = 0;
            p->popup_msg_handler = entry;
        //}
    }

    return err;
}


/*
 * Copy
 */
static void copy_msg(struct thread *dst_t, struct thread *src_t)
{
    msg_t *dst_msg = dst_t->memory.msg.start_paddr_ptr;
    msg_t *src_msg = src_t->memory.msg.start_paddr_ptr;

    // Size
    dst_msg->size = src_msg->size;

    //kprintf("copy, num params src: %lu\n", src_msg->num_params);

    // Sender
    dst_msg->sender.pid = src_t->pid;
    dst_msg->sender.tid = src_t->tid;

    // Params
    for (int i = 0; i < src_msg->num_params; i++) {
        dst_msg->params[i] = src_msg->params[i];
    }

    // Data
    if (src_msg->num_data_words) {
        void *dst_data = (void *)(dst_t->memory.msg.start_paddr_ptr + offsetof(msg_t, data));
        void *src_data = (void *)(src_t->memory.msg.start_paddr_ptr + offsetof(msg_t, data));
        memcpy(dst_data, src_data, src_msg->num_data_words * sizeof(ulong));
    }

    atomic_mb();
}


/*
 * Request
 */
int ipc_request(struct process *p, struct thread *t,
                ulong dst_pid, ulong opcode, ulong flags, int *wait)
{
    int err = -1;
    *wait = 0;

    //msg_t *msg = t->memory.msg.start_paddr_ptr;
    //kprintf("request, msg @ %p, num params: %lu\n", msg, msg->num_params);

    if (!dst_pid) {
        dst_pid = get_system_pid();
    }
    panic_if(!dst_pid, "Unknown PID: %lu\n", dst_pid);

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
                spinlock_lock_int(&ipc_wait_queue.lock);
                t->wait_type = IPC_WAIT_FOR_RESP;
            }

            create_and_run_thread(dst_tid, dst_t, dst_proc, popup_entry, opcode, NULL) {
                if (flags & IPC_SEND_WAIT_FOR_REPLY) {
                    t->ipc_wait.pid = dst_t->pid;
                    t->ipc_wait.tid = dst_t->tid;
                    dst_t->ipc_reply_to.pid = t->pid;
                    dst_t->ipc_reply_to.tid = t->tid;

                    ref_count_inc(&dst_t->ref_count);
                    ref_count_inc(&t->ref_count);
                }

                copy_msg(dst_t, t);
                err = 0;
                //kprintf("Popup msg from %lx -> %lx, popup entry @ %lx\n", t->tid, dst_tid, popup_entry);
            }
        }

        // Serial
        else if (flags & IPC_SEND_SERIAL) {
            spinlock_lock_int(&ipc_wait_queue.lock);

            struct thread *recv_t = NULL;

            list_foreach(&ipc_wait_queue, n) {
                struct thread *cur_t = list_entry(n, struct thread, node_wait);
                if (cur_t->wait_type == IPC_WAIT_FOR_SERIAL_REQ &&
                    cur_t->ipc_wait.pid == dst_pid
                ) {
                    recv_t = cur_t;
                    break;
                }
            }

            //kprintf("send serial, recv_t @ %p\n", recv_t);

            if (recv_t) {
                list_remove(&ipc_wait_queue, &recv_t->node_wait);

                if (flags & IPC_SEND_WAIT_FOR_REPLY) {
                    *wait = 1;
                    t->wait_type = IPC_WAIT_FOR_RESP;

                    t->ipc_wait.pid = recv_t->pid;
                    t->ipc_wait.tid = recv_t->tid;
                    recv_t->ipc_reply_to.pid = t->pid;
                    recv_t->ipc_reply_to.tid = t->tid;

                    ref_count_inc(&recv_t->ref_count);
                    ref_count_inc(&t->ref_count);
                } else {
                    *wait = 0;
                    spinlock_unlock_int(&ipc_wait_queue.lock);
                }

                copy_msg(recv_t, t);
                hal_set_syscall_return(&recv_t->context, 0, opcode, 0);

                wakeup_thread(recv_t);
            } else {
                *wait = 1;
                t->wait_type = IPC_WAIT_FOR_SERIAL_RECV;
                t->ipc_wait.pid = dst_pid;
                t->ipc_wait.tid = 0;
                t->ipc_wait.opcode = opcode;
                t->ipc_wait.need_response = flags & IPC_SEND_WAIT_FOR_REPLY ? 1 : 0;
            }

            err = 0;
        }
    }

    return err;
}


/*
 * Respond
 */
int ipc_respond(struct process *p, struct thread *t)
{
    int err = -1;
    ulong dst_pid = 0, dst_tid = 0;
    spinlock_exclusive_int(&t->lock) {
        dst_pid = t->ipc_reply_to.pid;
        dst_tid = t->ipc_reply_to.tid;
    }

    //kprintf("Responding to pid: %lx, tid: %lx\n", dst_pid, dst_tid);

    if (dst_pid && dst_tid) {
        struct thread *dst_t = NULL;

        list_foreach_exclusive(&ipc_wait_queue, n) {
            struct thread *cur_t = list_entry(n, struct thread, node_wait);
            if (cur_t->pid == dst_pid && cur_t->tid == dst_tid &&
                cur_t->wait_type == IPC_WAIT_FOR_RESP
            ) {
                dst_t = cur_t;
                list_remove(&ipc_wait_queue, &dst_t->node_wait);
                break;
            }
        }

        if (dst_t) {
            //list_remove_exclusive(&ipc_wait_queue, &dst_t->node_wait);

            copy_msg(dst_t, t);
            ref_count_dec(&dst_t->ref_count);
            ref_count_dec(&t->ref_count);

            wakeup_thread(dst_t);
            err = 0;
        }
    }

    return err;
}

int ipc_receive(struct process *p, struct thread *t, ulong timeout_ms,
                ulong *opcode, int *wait)
{
    int err = -1;

    spinlock_lock_int(&ipc_wait_queue.lock);

    struct thread *sender_t = NULL;

    list_foreach(&ipc_wait_queue, n) {
        struct thread *cur_t = list_entry(n, struct thread, node_wait);
        if (cur_t->wait_type == IPC_WAIT_FOR_SERIAL_RECV &&
            cur_t->ipc_wait.pid == p->pid
        ) {
            sender_t = cur_t;
            break;
        }
    }

    //kprintf("Serial receiving, sender @ %p\n", sender_t);

    if (sender_t) {
        *wait = 0;
        list_remove(&ipc_wait_queue, &sender_t->node_wait);

        copy_msg(t, sender_t);
        *opcode = sender_t->ipc_wait.opcode;

        if (sender_t->ipc_wait.need_response) {
            sender_t->wait_type = IPC_WAIT_FOR_RESP;
            sender_t->ipc_wait.pid = t->pid;
            sender_t->ipc_wait.tid = t->tid;

            t->ipc_reply_to.pid = sender_t->pid;
            t->ipc_reply_to.tid = sender_t->tid;

            ref_count_inc(&sender_t->ref_count);
            ref_count_inc(&t->ref_count);
        } else {
            hal_set_syscall_return(&sender_t->context, 0,
                                   sender_t->ipc_wait.opcode, 0);
            wakeup_thread(sender_t);
        }

        spinlock_unlock_int(&ipc_wait_queue.lock);
    } else {
        *wait = 1;

        t->wait_type = IPC_WAIT_FOR_SERIAL_REQ;
        t->ipc_wait.pid = t->pid;
        t->ipc_wait.tid = t->tid;
    }

    err = 0;

    return err;
}


/*
 * Purge
 */
ulong purge_ipc_queue(struct process *p)
{
    ulong count = 0;

    list_access_exclusive(&ipc_wait_queue) {
        list_foreach(&ipc_wait_queue, node) {
            struct thread *wait_t = list_entry(node, struct thread, node_wait);
            if (wait_t->pid == p->pid) {
                list_remove_in_foreach(&ipc_wait_queue, node);
                ref_count_dec(&wait_t->ref_count);

                if (wait_t->wait_type == IPC_WAIT_FOR_RESP) {
                    ulong sender_tid = wait_t->ipc_wait.tid;
                    if (sender_tid) {
                        access_thread(sender_tid, sender_t) {
                            spinlock_exclusive_int(&sender_t->lock) {
                                sender_t->ipc_reply_to.pid = 0;
                                sender_t->ipc_reply_to.tid = 0;
                            }
                            ref_count_dec(&sender_t->ref_count);
                        }
                    }
                }

                wakeup_thread(wait_t);
                count++;
            }
        }
    }

    return count;
}


/*
 * Notification to system
 */
int ipc_notify_system(ulong opcode, ulong pid)
{
    int err = -1;
    ulong dst_pid = get_system_pid();
    panic_if(!dst_pid, "Unknown PID: %lu\n", dst_pid);

    access_process(dst_pid, dst_proc) {
        ulong popup_entry = 0;
        spinlock_exclusive_int(&dst_proc->lock) {
            popup_entry = dst_proc->popup_msg_handler;
        }

        if (popup_entry) {
            create_and_run_thread(dst_tid, dst_t, dst_proc, popup_entry, opcode, NULL) {
                msg_t *msg = dst_t->memory.msg.start_paddr_ptr;
                clear_msg(msg);
                msg->sender.pid = 0;
                msg->sender.tid = 0;
                msg_append_param(msg, pid);
                atomic_mb();
                err = 0;
            }
        }
    }

    return err;
}
