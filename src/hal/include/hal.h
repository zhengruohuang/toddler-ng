#ifndef __HAL_INCLUDE_HAL_H__
#define __HAL_INCLUDE_HAL_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "common/include/mem.h"
#include "loader/include/export.h"
#include "hal/include/export.h"
#include "hal/include/dispatch.h"


typedef ulong (*ioport_read_t)(ulong port, int size);
typedef void (*ioport_write_t)(ulong port, int size, ulong val);

// typedef ppfn_t (*palloc_t)(int count);
// typedef int (*map_range_t)(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
//                      int cache, int exec, int write, int kernel, int override);

struct hal_arch_funcs {
    // Inits
    void (*init_libk)();
    void (*init_arch)();
    void (*init_arch_mp)();
    void (*init_int)();
    void (*init_int_mp)();
    void (*init_mm)();
    void (*init_mm_mp)();
    void (*init_kernel_pre)();
    void (*init_kernel_post)();

    // General
    putchar_t putchar;
    void (*halt)(int count, va_list args);

    // Indicates if direct physical address access is possible
    int has_direct_access, has_direct_access_uncached;
    direct_paddr_to_vaddr_t direct_paddr_to_vaddr;
    direct_vaddr_to_paddr_t direct_vaddr_to_paddr;

    // IO port
    int has_ioport;
    ioport_read_t ioport_read;
    ioport_write_t ioport_write;

    // Map and unmap
    map_range_t hal_map_range;
    map_range_t kernel_map_range;
    unmap_range_t kernel_unmap_range;
    page_translate_t translate;

    // Get CPU index
    get_cur_mp_id_t get_cur_mp_id;

    // Bring up secondary CPU
    ulong mp_entry;
    void (*start_cpu)(int mp_seq, ulong mp_id, ulong entry);

    // Dev
    void (*register_drivers)();

    // Syscall
    ulong (*get_syscall_params)(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2);
    set_syscall_return_t set_syscall_return;
    int (*handle_arch_syscall)(ulong num, ulong param0, ulong param1, ulong param2, ulong *return0, ulong *return1);

    // Int
    void (*arch_disable_local_int)();
    void (*arch_enable_local_int)();

    // Address space
    ulong vaddr_limit;
    ulong asid_limit;   // 0 indicates ASID not supported
    init_addr_space_t init_addr_space;
    free_addr_space_t free_addr_space;

    // Context
    init_context_t init_context;
    set_context_param_t set_context_param;
    switch_context_t switch_to;

    // Kernel dispatch
    void (*kernel_pre_dispatch)(ulong thread_id, struct kernel_dispatch *kdi);
    void (*kernel_post_dispatch)(ulong thread_id, struct kernel_dispatch *kdi);

    // TLB
    int has_auto_tlb_flush_on_switch;
    invalidate_tlb_t invalidate_tlb;
    flush_tlb_t flush_tlb;
};


/*
 * Loader and arch funcs
 */
extern struct loader_args *get_loader_args();
extern struct hal_arch_funcs *get_hal_arch_funcs();

extern void arch_init_kernel_pre();
extern void arch_init_kernel_post();

extern ulong arch_hal_direct_access(paddr_t paddr, int count, int cache);

extern int arch_hal_has_io_port();
extern ulong arch_hal_ioport_read(ulong port, int size);
extern void arch_hal_ioport_write(ulong port, int size, ulong val);

extern ulong arch_get_cur_mp_id();

extern void arch_start_cpu(int mp_seq, ulong mp_id, ulong entry);

extern void arch_register_drivers();

extern ulong arch_get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2);
extern void arch_set_syscall_return(struct reg_context *regs, int succeed, ulong return0, ulong return1);
extern int arch_handle_syscall(ulong num, ulong param0, ulong param1, ulong param2, ulong *return0, ulong *return1);

extern void arch_disable_local_int();
extern void arch_enable_local_int();

extern void arch_switch_to(ulong thread_id, struct reg_context *context,
                           void *page_table, int user_mode, ulong asid, ulong tcb);

extern int arch_has_auto_tlb_flush_on_switch();
extern void arch_flush_tlb();

extern void arch_kernel_pre_dispatch(ulong sched_id, struct kernel_dispatch *kdi);
extern void arch_kernel_post_dispatch(ulong sched_id, struct kernel_dispatch *kdi);


/*
 * Entry
 */
extern void hal(struct loader_args *largs, struct hal_arch_funcs *funcs);
extern void hal_mp();


#endif
