#ifndef __LIBSYS_INCLUDE_SYSCALL__
#define __LIBSYS_INCLUDE_SYSCALL__


#include "common/include/inttypes.h"
#include "common/include/syscall.h"
#include "common/include/sysenter.h"
#include "common/include/proc.h"


/*
 * Debug
 */
extern void syscall_puts(const char *buf, size_t len);


/*
 * Thread info block
 */
extern thread_info_block_t *syscall_get_tib();

static inline pid_t syscall_get_pid()
{
    thread_info_block_t *tib = syscall_get_tib();
    return tib ? tib->pid : 0;
}

static inline tid_t syscall_get_tid()
{
    thread_info_block_t *tib = syscall_get_tib();
    return tib ? tib->tid : 0;
}


/*
 * Interrupt
 */
extern ulong syscall_int_handler(ulong dev_fw_id, int dev_fw_pos, thread_entry_t entry);
extern ulong syscall_int_handler2(const char *dev_fw_path, int dev_fw_pos, thread_entry_t entry);
extern int syscall_int_eoi(ulong seq);


/*
 * Process
 */
extern pid_t syscall_process_create(int type);
extern int syscall_process_exit(pid_t pid, ulong status);
extern int syscall_process_recycle(pid_t pid);


/*
 * Thread
 */
extern ulong syscall_thread_create(thread_entry_t entry,  ulong param);
extern tid_t syscall_thread_create_cross(pid_t pid, ulong entry, ulong param);
extern void syscall_thread_yield();
extern void syscall_thread_exit_self(ulong status);


/*
 * VM
 */
extern ulong syscall_vm_alloc(ulong size, uint attri);
extern ulong syscall_vm_map(int type, ulong ppfn, ulong size);
extern ulong syscall_vm_map_cross(pid_t pid, ulong vbase, ulong size, ulong prot);
extern void syscall_vm_free(ulong base);

/*
 * Wait
 */
extern ulong syscall_wait_obj_alloc(void *user_obj, ulong total, ulong flags);

extern void syscall_wait_on_timeout(ulong timeout_ms);
extern void syscall_wait_on_futex(futex_t *f, ulong when);

extern void syscall_wake_on_futex(futex_t *f, ulong when);


/*
 * IPC
 */
extern void syscall_ipc_handler(thread_entry_t entry);
extern void syscall_ipc_serial_request(ulong pid, ulong opcode);
extern void syscall_ipc_serial_notify(ulong pid, ulong opcode);
extern void syscall_ipc_popup_request(ulong pid, ulong opcode);
extern void syscall_ipc_popup_notify(ulong pid, ulong opcode);
extern void syscall_ipc_respond();
extern void syscall_ipc_receive(ulong *opcode);


/*
 * Stats
 */
extern void syscall_stats_kernel(struct kernel_stats *buf);


#endif
