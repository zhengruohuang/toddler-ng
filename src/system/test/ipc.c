#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"
#include "libsys/include/ipc.h"


#define SEND_REPEAT 16


/*
 * Send
 */
struct send_param {
    int type;
    int repeat;
    ulong opcode_min, opcode_max;
    ulong msg_param;
};

static ulong test_ipc_send_worker(ulong param)
{
    struct send_param *info = (void *)param;
    ulong pid = syscall_get_tib()->pid;
    ulong tid = syscall_get_tib()->tid;

    msg_t *msg = get_msg();
    clear_msg(msg);
    msg_append_param(msg, pid);
    msg_append_param(msg, tid);
    msg_append_param(msg, info->msg_param);

    for (int i = 0; i < info->repeat; i++) {
        for (ulong opcode = info->opcode_min; opcode <= info->opcode_max; opcode++) {
            kprintf("Sending msg #%d, Opcode: %lx, Dest: %lx\n", i, opcode, pid);
            switch (info->type) {
            case 0:
                syscall_ipc_send(pid, opcode);
                break;
            case 1:
                syscall_ipc_request(pid, opcode);
                break;
            case 2:
                syscall_ipc_notify(pid, opcode);
                break;
            default:
                break;
            }
        }
    }

    return 0;
}


/*
 * Serial
 */
static void test_ipc_recv_serial(struct send_param *info)
{
    for (int i = 0; i < info->repeat; i++) {
        for (ulong opcode = info->opcode_min; opcode <= info->opcode_max; opcode++) {
            syscall_ipc_recv(NULL);
            kprintf("Received msg #%d\n", i);
        }
    }
}

static void test_ipc_serial()
{
    kprintf("Testing serial IPC\n");

    kthread_t sender;
    struct send_param info = {
        .type = 0, .repeat = SEND_REPEAT, .msg_param = 0,
        .opcode_min = 0, .opcode_max = 0
    };

    kthread_create(&sender, test_ipc_send_worker, (ulong)&info);
    test_ipc_recv_serial(&info);
    kthread_join(&sender, NULL);

    kprintf("Passed serial IPC!\n");
}


/*
 * Popup
 */
static ulong test_ipc_popup_handler(ulong opcode)
{
    kprintf("Received msg: %lx\n", opcode);
    syscall_ipc_respond();
    return 0;
}

static void test_ipc_popup()
{
    kprintf("Testing popup IPC\n");

    struct send_param info = {
        .type = 1, .repeat = SEND_REPEAT, .msg_param = 0,
        .opcode_min = 0, .opcode_max = 0
    };

    register_msg_handler(0, test_ipc_popup_handler);
    test_ipc_send_worker((ulong)&info);
    cancel_msg_handler(0);
}


/*
 * Notify
 */
static ulong test_ipc_notification_handler(ulong opcode)
{
    msg_t *msg = get_msg();
    ulong pid = msg_get_param(msg, 0);
    ulong tid = msg_get_param(msg, 1);
    ulong param = msg_get_param(msg, 2);

    volatile ulong *notif_recv_count = (void *)param;
    if (notif_recv_count) {
        ++*notif_recv_count;
        atomic_mb();
        atomic_notify();
        kprintf("notif_recv_count: %ld\n", *notif_recv_count);
    }

    kprintf("Received notif: %lx, from PID: %lx, TID: %lx, Param: %lx\n",
            opcode, pid, tid, param);
    return 0;
}

static void test_ipc_notify()
{
    kprintf("Testing notify IPC\n");

    volatile ulong notif_recv_count = 0;

    struct send_param info = {
        .type = 2, .repeat = SEND_REPEAT, .msg_param = (ulong)&notif_recv_count,
        .opcode_min = 0, .opcode_max = 0
    };

    register_msg_handler(0, test_ipc_notification_handler);
    test_ipc_send_worker((ulong)&info);

    ulong expected_notif_recv_count = (info.opcode_max - info.opcode_min + 1) * SEND_REPEAT;
    while (notif_recv_count != expected_notif_recv_count) {
        atomic_pause();
        atomic_mb();
    }
    cancel_msg_handler(0);
}


/*
 * Entry
 */
void test_ipc()
{
    //test_ipc_serial();
    test_ipc_popup();
    test_ipc_notify();
}
