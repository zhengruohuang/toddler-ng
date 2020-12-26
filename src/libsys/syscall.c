#include "common/include/inttypes.h"
#include "common/include/errno.h"
#include "common/include/proc.h"
#include "libk/include/debug.h"
#include "libsys/include/syscall.h"


void syscall_puts(const char *buf, size_t len)
{
    sysenter(SYSCALL_PUTS, (ulong)buf, (ulong)len, 0, NULL, NULL);
}

thread_info_block_t *syscall_get_tib()
{
#if (defined(FAST_GET_TIB) && FAST_GET_TIB)
    return (void *)sysenter_get_tib();
#else
    ulong tib = 0;
    sysenter(SYSCALL_HAL_GET_TIB, 0, 0, 0, &tib, NULL);
    return (void *)tib;
#endif
}


/*
 * Thread
 */
ulong syscall_thread_create(thread_entry_t entry,  ulong param)
{
    ulong tid = 0;
    sysenter(SYSCALL_THREAD_CREATE, (ulong)entry, param, 0, &tid, NULL);
    return tid;
}


void syscall_thread_yield()
{
    sysenter(SYSCALL_THREAD_YIELD, 0, 0, 0, NULL, NULL);
}

void syscall_thread_exit_self(ulong status)
{
    sysenter(SYSCALL_THREAD_EXIT, 0, status, 0, NULL, NULL);
    unreachable();
}


/*
 * VM
 */
ulong syscall_vm_alloc(ulong size, uint attri)
{
    ulong base = 0;
    sysenter(SYSCALL_VM_ALLOC, 0, size, attri, &base, NULL);
    return base;
}

void syscall_vm_free(ulong base)
{
    sysenter(SYSCALL_VM_FREE, base, 0, 0, NULL, NULL);
}


/*
 * Wait/Wake
 */
ulong syscall_wait_obj_alloc(void *user_obj, ulong total, ulong flags)
{
    ulong kernel_obj_id = 0;
    sysenter(SYSCALL_EVENT_ALLOC, (ulong)user_obj, total, flags, &kernel_obj_id, NULL);
    return kernel_obj_id;
}

void syscall_wait_on_timeout(ulong timeout_ms)
{
    sysenter(SYSCALL_EVENT_WAIT, WAIT_ON_TIMEOUT, 0, timeout_ms, NULL, NULL);
}

void syscall_wait_on_futex(futex_t *f, ulong skip)
{
    sysenter(SYSCALL_EVENT_WAIT, WAIT_ON_FUTEX, (ulong)f, skip, NULL, NULL);
}

// void syscall_wait_on_thread(ulong target_tid)
// {
//     //sysenter(SYSCALL_EVENT_WAIT, WAIT_ON_THREAD, target_tid, 0, NULL, NULL);
// }
//
// void syscall_wait_on_main_thread(ulong target_pid)
// {
//     //sysenter(SYSCALL_EVENT_WAIT, WAIT_ON_MAIN_THREAD, target_pid, 0, NULL, NULL);
// }
//
// void syscall_wait_on_process(ulong target_pid)
// {
//     //sysenter(SYSCALL_EVENT_WAIT, WAIT_ON_PROCESS, target_pid, 0, NULL, NULL);
// }

void syscall_wake_on_futex(futex_t *f, ulong skip)
{
    sysenter(SYSCALL_EVENT_WAKE, WAIT_ON_FUTEX, (ulong)f, skip, NULL, NULL);
}


/*
 * IPC
 */
void syscall_ipc_handler(thread_entry_t entry)
{
    sysenter(SYSCALL_IPC_HANDLER, (ulong)entry, 0, 0, NULL, NULL);
}

void syscall_ipc_serial_request(ulong pid, ulong opcode)
{
    sysenter(SYSCALL_IPC_REQUEST, pid, opcode, IPC_SEND_WAIT_FOR_REPLY | IPC_SEND_SERIAL, NULL, NULL);
}

void syscall_ipc_serial_notify(ulong pid, ulong opcode)
{
    sysenter(SYSCALL_IPC_REQUEST, pid, opcode, IPC_SEND_SERIAL, NULL, NULL);
}

void syscall_ipc_popup_request(ulong pid, ulong opcode)
{
    sysenter(SYSCALL_IPC_REQUEST, pid, opcode, IPC_SEND_WAIT_FOR_REPLY | IPC_SEND_SERIAL_POPUP, NULL, NULL);
}

void syscall_ipc_popup_notify(ulong pid, ulong opcode)
{
    sysenter(SYSCALL_IPC_REQUEST, pid, opcode, IPC_SEND_SERIAL_POPUP, NULL, NULL);
}

void syscall_ipc_respond()
{
    sysenter(SYSCALL_IPC_RESPOND, 0, 0, 0, NULL, NULL);
}

void syscall_ipc_receive(ulong *opcode)
{
    sysenter(SYSCALL_IPC_RECEIVE, 0, 0, 0, opcode, NULL);
}
