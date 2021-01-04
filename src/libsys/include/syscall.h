#ifndef __LIBSYS_INCLUDE_SYSCALL__
#define __LIBSYS_INCLUDE_SYSCALL__


#include "common/include/inttypes.h"
#include "common/include/syscall.h"
#include "common/include/sysenter.h"
#include "common/include/proc.h"


extern void syscall_puts(const char *buf, size_t len);


/*
 * Thread info block
 */
extern thread_info_block_t *syscall_get_tib();


/*
 * Thread
 */
extern ulong syscall_thread_create(thread_entry_t entry,  ulong param);
extern void syscall_thread_yield();
extern void syscall_thread_exit_self(ulong status);


/*
 * VM
 */
extern ulong syscall_vm_alloc(ulong size, uint attri);
extern ulong syscall_vm_map(int type, ulong ppfn, ulong size);
extern void syscall_vm_free(ulong base);

/*
 * Wait
 */
extern ulong syscall_wait_obj_alloc(void *user_obj, ulong total, ulong flags);

extern void syscall_wait_on_timeout(ulong timeout_ms);
extern void syscall_wait_on_futex(futex_t *f, ulong skip);
// extern void syscall_wait_on_thread(ulong target_tid);
// extern void syscall_wait_on_main_thread(ulong target_pid);
// extern void syscall_wait_on_process(ulong target_pid);

extern void syscall_wake_on_futex(futex_t *f, ulong skip);


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


#endif
