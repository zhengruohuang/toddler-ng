#include "common/include/inttypes.h"
#include "common/include/errno.h"
#include "common/include/proc.h"
#include "libk/include/debug.h"
#include "libsys/include/syscall.h"


/*
 * Kputs
 */
void syscall_puts(const char *buf, size_t len)
{
    sysenter(SYSCALL_PUTS, (ulong)buf, (ulong)len, 0, NULL, NULL);
}


/*
 * Thread info block
 */
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
 * Interrupt
 */
ulong syscall_int_handler(ulong intc_phandle, thread_entry_t entry)
{
    ulong seq = -1ul;
    sysenter(SYSCALL_INT_HANDLER, intc_phandle, (ulong)entry, 0, &seq, NULL);
    return seq;
}

int syscall_int_eoi(ulong seq)
{
    int err = sysenter(SYSCALL_INT_EOI, seq, 0, 0, NULL, NULL);
    return err;
}


/*
 * Process
 */
pid_t syscall_process_create(int type)
{
    pid_t pid = 0;
    sysenter(SYSCALL_PROCESS_CREATE, type, 0, 0, &pid, NULL);
    return pid;
}

int syscall_process_exit(pid_t pid, ulong status)
{
    int err = sysenter(SYSCALL_PROCESS_EXIT, pid, status, 0, NULL, NULL);
    return err;
}

int syscall_process_recycle(pid_t pid)
{
    int err = sysenter(SYSCALL_PROCESS_RECYCLE, pid, 0, 0, NULL, NULL);
    return err;
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

tid_t syscall_thread_create_cross(pid_t pid, ulong entry, ulong param)
{
    ulong tid = 0;
    sysenter(SYSCALL_THREAD_CREATE_CROSS, pid, entry, param, &tid, NULL);
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

ulong syscall_vm_map(int type, ulong ppfn, ulong count)
{
    ulong vbase = 0;
    sysenter(SYSCALL_VM_MAP, type, ppfn, count, &vbase, NULL);
    return vbase;
}

ulong syscall_vm_map_cross(pid_t pid, ulong vbase, ulong size, ulong prot)
{
    ulong local_vbase = 0;
    sysenter(SYSCALL_VM_MAP_CROSS, pid, vbase, size, &local_vbase, NULL);
    return local_vbase;
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

void syscall_wait_on_futex(futex_t *f, ulong when)
{
    sysenter(SYSCALL_EVENT_WAIT, WAIT_ON_FUTEX, (ulong)f, when, NULL, NULL);
}

void syscall_wake_on_futex(futex_t *f, ulong when)
{
    sysenter(SYSCALL_EVENT_WAKE, WAIT_ON_FUTEX, (ulong)f, when, NULL, NULL);
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


/*
 * Stats
 */
void syscall_stats_kernel(struct kernel_stats *buf)
{
    sysenter(SYSCALL_STATS_KERNEL, (ulong)buf, 0, 0, NULL, NULL);
}
