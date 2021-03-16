/*
 * System-class syscall handlers
 */


#include "common/include/inttypes.h"
#include "kernel/include/kprintf.h"
#include "kernel/include/proc.h"
#include "kernel/include/lib.h"
#include "kernel/include/mem.h"
#include "kernel/include/kernel.h"
#include "kernel/include/syscall.h"


/*
 * None
 */
int syscall_handler_none(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    hal_set_syscall_return(kdi->regs, 0, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}


/*
 * Ping
 */
int syscall_handler_ping(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    hal_set_syscall_return(kdi->regs, 0, kdi->param0 + 1, kdi->param1 + 1);

    ulong ret_type = kdi->param2;
    switch (ret_type) {
    case 0: return SYSCALL_HANDLED_CONTINUE;
    case 1: return SYSCALL_HANDLED_SAVE_CONTINUE;
    case 2: return SYSCALL_HANDLED_SAVE_RESCHED;
    default: return SYSCALL_HANDLED_CONTINUE;
    }

    return SYSCALL_HANDLED_CONTINUE;
}


/*
 * Puts
 */
#define PUTS_BUF_SIZE   64
#define PUTS_MAX_LEN    8192

int syscall_handler_puts(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    static char buf[PUTS_BUF_SIZE + 1];

    ulong len = kdi->param1;
    if (len > PUTS_MAX_LEN) {
        len = PUTS_MAX_LEN;
    }

    acquire_kprintf();

    for (ulong vaddr = kdi->param0, copied_len = 0; copied_len < len;
         copied_len += PUTS_BUF_SIZE, vaddr += PUTS_BUF_SIZE
    ) {
        ulong cur_len = (copied_len + PUTS_BUF_SIZE) <= len ?
                            PUTS_BUF_SIZE : (len - copied_len);

        ulong vaddr_last_byte = vaddr + cur_len - 1;

        // String fit in one page
        if (align_down_vaddr(vaddr, PAGE_SIZE) ==
            align_down_vaddr(vaddr_last_byte, PAGE_SIZE)
        ) {
            paddr_t paddr = get_hal_exports()->translate(p->page_table, vaddr);
            void *paddr_ptr = hal_cast_paddr_to_kernel_ptr(paddr);
            memcpy(buf, paddr_ptr, cur_len);
            buf[cur_len] = '\0';
            kprintf_unlocked("%s", buf);
        }

        // Cross page boundary
        else {
            ulong part_len = align_up_vaddr(vaddr, PAGE_SIZE) - vaddr;
            paddr_t paddr = get_hal_exports()->translate(p->page_table, vaddr);
            void *paddr_ptr = hal_cast_paddr_to_kernel_ptr(paddr);
            memcpy(buf, paddr_ptr, part_len);
            buf[part_len] = '\0';
            kprintf_unlocked("%s", buf);

            ulong vaddr2 = vaddr + part_len;
            part_len = cur_len - part_len;
            paddr = get_hal_exports()->translate(p->page_table, vaddr2);
            paddr_ptr = hal_cast_paddr_to_kernel_ptr(paddr);
            memcpy(buf, paddr_ptr, part_len);
            buf[part_len] = '\0';
            kprintf_unlocked("%s", buf);
        }
    }

    release_kprintf();

    hal_set_syscall_return(kdi->regs, 0, len, 0);
    return SYSCALL_HANDLED_CONTINUE;
}


/*
 * Illegal
 */
int syscall_handler_illegal(struct process *p, struct thread *t,
                            struct kernel_dispatch *kdi)
{
    kprintf("Illegal syscall: %d\n", kdi->num);

    // TODO: start error handler
    return SYSCALL_HANDLED_SKIP;
}


/*
 * Interrupt
 */
int syscall_handler_interrupt(struct process *p, struct thread *t,
                              struct kernel_dispatch *kdi)
{
    ulong seq = kdi->param0;
    if (seq) {
        //kprintf("Kernel received interrupt: %lu!\n", kdi->param0);
        int_handler_invoke(seq);
    }

    return SYSCALL_HANDLED_SAVE_RESCHED;
}

int syscall_handler_int_register(struct process *p, struct thread *t,
                                 struct kernel_dispatch *kdi)
{
    ulong fw_id = kdi->param0;
    ulong fw_pos = kdi->param1;
    ulong entry = kdi->param2;
    ulong seq = int_handler_register(p, fw_id, fw_pos, entry);

    int err = seq == -1ul ? -1 : 0;
    hal_set_syscall_return(kdi->regs, err, seq, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_int_register2(struct process *p, struct thread *t,
                                  struct kernel_dispatch *kdi)
{
    ulong fw_path_vaddr = kdi->param0;
    paddr_t fw_path_paddr = get_hal_exports()->translate(p->page_table, fw_path_vaddr);
    void *fw_path_ptr = hal_cast_paddr_to_kernel_ptr(fw_path_paddr);

    ulong fw_pos = kdi->param1;
    ulong entry = kdi->param2;
    ulong seq = int_handler_register2(p, (const char *)fw_path_ptr, fw_pos, entry);

    int err = seq == -1ul ? -1 : 0;
    hal_set_syscall_return(kdi->regs, err, seq, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_int_eoi(struct process *p, struct thread *t,
                            struct kernel_dispatch *kdi)
{
    ulong seq = kdi->param0;
    int err = int_handler_eoi(seq);

    hal_set_syscall_return(kdi->regs, err, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}


/*
 * Fault
 */
int syscall_handler_fault_page(struct process *p, struct thread *t,
                               struct kernel_dispatch *kdi)
{
    //kprintf("Page fault @ %lx\n", kdi->param0);
    int err = vm_map(p, kdi->param0, 0);

    return err ? SYSCALL_HANDLED_SAVE_RESCHED : SYSCALL_HANDLED_CONTINUE;
}


/*
 * Process
 */
int syscall_handler_process_create(struct process *p, struct thread *t,
                                   struct kernel_dispatch *kdi)
{
    int type = kdi->param0;

    // TODO: Name
    ulong pid = create_process(p->pid, "my_proc", type);

    hal_set_syscall_return(kdi->regs, 0, pid, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_process_exit(struct process *p, struct thread *t,
                                 struct kernel_dispatch *kdi)
{
    ulong pid = kdi->param0;
    ulong status = kdi->param1;

    int err = -1;
    access_process(pid, proc) {
        err = exit_process(proc, status);
    }

    hal_set_syscall_return(kdi->regs, err, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_process_recycle(struct process *p, struct thread *t,
                                    struct kernel_dispatch *kdi)
{
    ulong pid = kdi->param0;

    int err = -1;
    access_process(pid, proc) {
        err = recycle_process(proc);
    }

    hal_set_syscall_return(kdi->regs, err, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}


/*
 * VM
 */
int syscall_handler_vm_alloc(struct process *p, struct thread *t,
                             struct kernel_dispatch *kdi)
{
    ulong base = kdi->param0;
    ulong size = kdi->param1;
    ulong attri = kdi->param2;

    struct vm_block *b = vm_alloc(p, base, size, attri);
    if (b) {
        b->map_type = VM_MAP_TYPE_OWNER;
        base = b->base;
    } else {
        base = 0;
    }

    hal_set_syscall_return(kdi->regs, 0, base, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_vm_map(struct process *p, struct thread *t,
                           struct kernel_dispatch *kdi)
{
    int type = kdi->param0;
    ulong ppfn = kdi->param1;
    ulong size = kdi->param2;

    ulong vbase = 0;
    switch (type) {
    case VM_MAP_COREIMG:
        vbase = vm_map_coreimg(p);
        break;
    case VM_MAP_DEVTREE:
        vbase = vm_map_devtree(p);
        break;
    case VM_MAP_DEV:
        vbase = vm_map_dev(p, ppfn, size, 0);
        break;
    case VM_MAP_KERNEL:
        vbase = vm_map_kernel(p, size, 0);
        break;
    default:
        break;
    }

    hal_set_syscall_return(kdi->regs, 0, vbase, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_vm_map_cross(struct process *p, struct thread *t,
                                 struct kernel_dispatch *kdi)
{
    ulong pid = kdi->param0;
    ulong remote_vbase = kdi->param1;
    ulong size = kdi->param2;

    ulong local_vbase = vm_map_cross(p, pid, remote_vbase, size, 0);

    hal_set_syscall_return(kdi->regs, 0, local_vbase, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_vm_free(struct process *p, struct thread *t,
                            struct kernel_dispatch *kdi)
{
    ulong base = kdi->param0;
    vm_free(p, base);

    hal_set_syscall_return(kdi->regs, 0, 0, 0);
    return SYSCALL_HANDLED_SAVE_RESCHED;
}


/*
 * Thread
 */
int syscall_handler_thread_create(struct process *p, struct thread *t,
                                  struct kernel_dispatch *kdi)
{
    ulong entry = kdi->param0;
    ulong param = kdi->param1;

    ulong ret_tid = 0;
    create_and_run_thread(tid, t, p, entry, param, NULL) {
        ret_tid = tid;
    }

    hal_set_syscall_return(kdi->regs, 0, ret_tid, 0);
    return SYSCALL_HANDLED_SAVE_RESCHED;
}

int syscall_handler_thread_create_cross(struct process *p, struct thread *t,
                                        struct kernel_dispatch *kdi)
{
    ulong pid = kdi->param0;
    ulong entry = kdi->param1;
    ulong param = kdi->param2;

    ulong ret_tid = 0;
    access_process(pid, dst_proc) {
        if (dst_proc->state == PROCESS_STATE_ENTER) {
            create_and_run_thread(tid, dst_thrd, dst_proc, entry, param, NULL) {
                ret_tid = tid;
            }
        }
    }

    hal_set_syscall_return(kdi->regs, 0, ret_tid, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_thread_yield(struct process *p, struct thread *t,
                                 struct kernel_dispatch *kdi)
{
    hal_set_syscall_return(kdi->regs, 0, 0, 0);
    return SYSCALL_HANDLED_SAVE_RESCHED;
}

int syscall_handler_thread_exit(struct process *p, struct thread *t,
                                struct kernel_dispatch *kdi)
{
    hal_set_syscall_return(kdi->regs, 0, 0, 0);
    return SYSCALL_HANDLED_EXIT_THREAD;
}


/*
 * Wait
 */
int syscall_handler_wait(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    int wait_type = kdi->param0;
    ulong wait_obj = kdi->param1;
    ulong wait_value = kdi->param2;
    ulong timeout_ms = kdi->param2;

    int err = 0;
    switch (wait_type) {
    case WAIT_ON_TIMEOUT:
        err = wait_on_object(p, t, wait_type, wait_obj, 0, timeout_ms);
        break;
    case WAIT_ON_FUTEX:
        err = wait_on_object(p, t, wait_type, wait_obj, wait_value, 0);
        break;
    default:
        break;
    }

    hal_set_syscall_return(kdi->regs, 0, (ulong)err, 0);
    return err ? SYSCALL_HANDLED_CONTINUE : SYSCALL_HANDLED_SAVE_SLEEP;
}

int syscall_handler_wake(struct process *p, struct thread *t,
                         struct kernel_dispatch *kdi)
{
    int wait_type = kdi->param0;
    ulong wait_obj = kdi->param1;
    ulong wait_value = kdi->param2;
    ulong max_count = kdi->param2;

    ulong count = 0;
    switch (wait_type) {
    case WAIT_ON_TIMEOUT:
        count = wake_on_object_exclusive(p, t, wait_type, wait_obj, 0, max_count);
        break;
    case WAIT_ON_FUTEX:
        count = wake_on_object_exclusive(p, t, wait_type, wait_obj, wait_value, 0);
        break;
    default:
        break;
    }

    hal_set_syscall_return(kdi->regs, 0, count, 0);
    return SYSCALL_HANDLED_CONTINUE;
}


/*
 * IPC
 */
int syscall_handler_ipc_register(struct process *p, struct thread *t,
                                 struct kernel_dispatch *kdi)
{
    ulong popup_entry = kdi->param0;
    int err = ipc_reg_popup_handler(p, t, popup_entry);
    hal_set_syscall_return(kdi->regs, err, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_ipc_request(struct process *p, struct thread *t,
                                struct kernel_dispatch *kdi)
{
    ulong dst_pid = kdi->param0;
    ulong opcode = kdi->param1;
    ulong flags = kdi->param2;

    int wait = 0;
    int err = ipc_request(p, t, dst_pid, opcode, flags, &wait);

    hal_set_syscall_return(kdi->regs, err, 0, 0);
    return wait ? SYSCALL_HANDLED_SAVE_IPC : SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_ipc_respond(struct process *p, struct thread *t,
                                struct kernel_dispatch *kdi)
{
    int err = ipc_respond(p, t);
    hal_set_syscall_return(kdi->regs, err, 0, 0);
    return SYSCALL_HANDLED_CONTINUE;
}

int syscall_handler_ipc_receive(struct process *p, struct thread *t,
                                struct kernel_dispatch *kdi)
{
    ulong timeout_ms = kdi->param0;

    ulong opcode = 0;
    int wait = 0;
    int err = ipc_receive(p, t, timeout_ms, &opcode, &wait);

    hal_set_syscall_return(kdi->regs, err, opcode, 0);
    return wait ? SYSCALL_HANDLED_SAVE_IPC : SYSCALL_HANDLED_CONTINUE;
}
