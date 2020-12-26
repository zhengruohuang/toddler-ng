#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"
#include "libsys/include/ipc.h"





/*
 * Test setup
 */
#define SEND_REPEAT 16

enum test_type {
    TEST_SERIAL_REQUEST,
    TEST_SERIAL_NOTIFY,
    TEST_POPUP_REQUEST,
    TEST_POPUP_NOTIFY,
};

struct send_param {
    int type;
    int repeat;
    ulong opcode_min, opcode_max;

    volatile ulong receive_count;
    ulong response_count;
    ulong expected_count;
};

#define test_ipc_foreach(info, i, opcode) \
    for (int i = 0; i < info->repeat; i++) \
        for (ulong opcode = info->opcode_min; opcode <= info->opcode_max; opcode++)


/*
 * Send
 */
static ulong test_ipc_send_worker(ulong param)
{
    struct send_param *info = (void *)param;
    ulong pid = syscall_get_tib()->pid;
    ulong tid = syscall_get_tib()->tid;

    test_ipc_foreach(info, i, opcode) {
        msg_t *msg = get_msg();
        clear_msg(msg);
        msg_append_param(msg, pid);
        msg_append_param(msg, tid);
        msg_append_param(msg, (ulong)(void *)&info->receive_count);
        msg_append_param(msg, opcode);
        msg_append_param(msg, info->type);

        kprintf("Sending msg #%d, Opcode: %lx, Dest: %lx, Num params: %lu\n",
                i, opcode, pid, msg->num_params);

        switch (info->type) {
        case TEST_SERIAL_REQUEST:
            syscall_ipc_serial_request(pid, opcode);
            break;
        case TEST_SERIAL_NOTIFY:
            syscall_ipc_serial_notify(pid, opcode);
            break;
        case TEST_POPUP_REQUEST:
            syscall_ipc_popup_request(pid, opcode);
            break;
        case TEST_POPUP_NOTIFY:
            syscall_ipc_popup_notify(pid, opcode);
            break;
        default:
            kprintf("Unknown test type: %d!\n", info->type);
            break;
        }

        switch (info->type) {
        case TEST_SERIAL_REQUEST:
        case TEST_POPUP_REQUEST: {
            msg_t *msg = get_msg();
            //ulong pid = msg_get_param(msg, 0);
            //ulong tid = msg_get_param(msg, 1);
            ulong response_count = msg_get_param(msg, 2);
            info->response_count += response_count;
            break;
        }
        case TEST_SERIAL_NOTIFY:
        case TEST_POPUP_NOTIFY:
        default:
            break;
        }

        info->expected_count += opcode;
    }

    return 0;
}


/*
 * Receive
 */
static void test_ipc_receive(ulong opcode)
{
    msg_t *msg = get_msg();
    ulong sender_pid = msg_get_param(msg, 0);
    ulong sender_tid = msg_get_param(msg, 1);
    ulong recv_count = msg_get_param(msg, 2);
    ulong msg_opcode = msg_get_param(msg, 3);
    int send_type = msg_get_param(msg, 4);

    if (msg_opcode != opcode) {
        kprintf("Msg opcode mismatch!\n");
    }

    if (recv_count) {
        volatile ulong *recv_count_ptr = (void *)recv_count;
        atomic_fetch_and_add(recv_count_ptr, opcode);
        atomic_mb();
        atomic_notify();
    }

    switch (send_type) {
    case TEST_SERIAL_REQUEST:
    case TEST_POPUP_REQUEST: {
        ulong pid = syscall_get_tib()->pid;
        ulong tid = syscall_get_tib()->tid;

        if (sender_pid != pid) {
            kprintf("Sender pid mismatch!\n");
        }
        if (sender_tid == tid) {
            kprintf("Sender tid shouldn't be the same!\n");
        }

        msg_t *resp_msg = get_msg();
        clear_msg(resp_msg);
        msg_append_param(resp_msg, pid);
        msg_append_param(resp_msg, tid);
        msg_append_param(resp_msg, opcode);

        syscall_ipc_respond();
        break;
    }
    case TEST_SERIAL_NOTIFY:
    case TEST_POPUP_NOTIFY:
        break;
    default:
        kprintf("Unknown test type: %d\n", send_type);
        break;
    }
}


/*
 * Validate
 */
static int test_ipc_validate(struct send_param *info)
{
    int err = 0;

    switch (info->type) {
    case TEST_SERIAL_REQUEST:
    case TEST_POPUP_REQUEST:
        if (info->expected_count != info->receive_count) {
            kprintf("Receive count mismatch!\n");
            err = -1;
        }
        if (info->expected_count != info->response_count) {
            kprintf("Response count mismatch!\n");
            err = -1;
        }
        break;
    case TEST_SERIAL_NOTIFY:
    case TEST_POPUP_NOTIFY: {
        ulong loop_count = 0;
        while (info->expected_count != info->receive_count) {
            atomic_pause();
            atomic_mb();
            if (loop_count++ >= 1000000) {
                err = -1;
                break;
            }
            syscall_thread_yield();
        }
        break;
    }
    default:
        kprintf("Unknown test type: %d\n", info->type);
        break;
    }

    return err;
}


/*
 * Serial
 */
static void test_ipc_recv_serial(struct send_param *info)
{
    test_ipc_foreach(info, i, opcode) {
        ulong recv_opcode = 0;
        syscall_ipc_receive(&recv_opcode);

        if (recv_opcode != opcode) {
            kprintf("Serial recv opcode mismatch!\n");
        }

        test_ipc_receive(opcode);
    }
}

static void test_ipc_serial(int type, const char *name)
{
    kprintf("Testing serial IPC: %s\n", name);

    kthread_t sender;
    struct send_param info = {
        .type = type, .repeat = SEND_REPEAT,
        .opcode_min = 1, .opcode_max = 2
    };

    kthread_create(&sender, test_ipc_send_worker, (ulong)&info);
    test_ipc_recv_serial(&info);
    kthread_join(&sender, NULL);

    int err = test_ipc_validate(&info);
    kprintf("%s serial IPC tests: %s!\n", err ? "Failed" : "Passed", name);
}

static void test_ipc_serial_request()
{
    test_ipc_serial(TEST_SERIAL_REQUEST, "request");
}

static void test_ipc_serial_notify()
{
    test_ipc_serial(TEST_SERIAL_NOTIFY, "notify");
}


/*
 * Popup
 */
static ulong test_ipc_popup_handler(ulong opcode)
{
    test_ipc_receive(opcode);
    return 0;
}

static void test_ipc_popup(int type, const char *name)
{
    kprintf("Testing popup IPC: %s\n", name);

    struct send_param info = {
        .type = type, .repeat = SEND_REPEAT,
        .opcode_min = 1, .opcode_max = 2
    };

    for (int i = info.opcode_min; i <= info.opcode_max; i++) {
        register_msg_handler(i, test_ipc_popup_handler);
    }

    test_ipc_send_worker((ulong)&info);

    int err = test_ipc_validate(&info);
    kprintf("%s popup IPC tests: %s!\n", err ? "Failed" : "Passed", name);

    for (int i = info.opcode_min; i <= info.opcode_max; i++) {
        cancel_msg_handler(i);
    }
}

static void test_ipc_popup_request()
{
    test_ipc_popup(TEST_POPUP_REQUEST, "request");
}

static void test_ipc_popup_notify()
{
    test_ipc_popup(TEST_POPUP_NOTIFY, "notify");
}


/*
 * Entry
 */
void test_ipc()
{
    test_ipc_serial_request();
    test_ipc_serial_notify();
    test_ipc_popup_request();
    test_ipc_popup_notify();
}
