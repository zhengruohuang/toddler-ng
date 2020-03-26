#ifndef __HAL_INCLUDE_HAL_H__
#define __HAL_INCLUDE_HAL_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "loader/include/export.h"
#include "hal/include/export.h"
#include "hal/include/dispatch.h"


typedef ulong (*palloc_t)(int count);
typedef int (*generic_map_range_t)(void *page_table, ulong vaddr, ulong paddr, ulong size,
                     int cache, int exec, int write, int kernel, int override,
                     palloc_t palloc);

struct hal_arch_funcs {
    // Inits
    void (*init_libk)();
    void (*init_arch)();
    void (*init_int)();
    void (*init_mm)();

    // General
    putchar_t putchar;
    void (*halt)(int count, va_list args);

    // Indicates if direct physical address access is possible
    int has_direct_access;
    ulong (*hal_direct_access)(ulong paddr, int count, int cache);

    // Map and unmap
    generic_map_range_t map_range;
    unmap_range_t unmap_range;
    page_translate_t translate;

    // Get CPU index
    get_cur_cpu_index_t get_cur_cpu_index;

    // Bring up secondary CPU
    void (*bringup_cpu)(int index);

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
    init_addr_space_t init_addr_space;

    // Context
    init_context_t init_context;
    set_context_param_t set_context_param;
    switch_context_t switch_to;
    void (*kernel_dispatch_prep)(ulong sched_id, struct kernel_dispatch_info *kdi);

    // TLB
    invalidate_tlb_t invalidate_tlb;
};


extern struct loader_args *get_loader_args();
extern struct hal_arch_funcs *get_hal_arch_funcs();

extern ulong arch_hal_direct_access(ulong paddr, int count, int cache);
extern int arch_get_cpu_index();
extern void arch_bringup_cpu(int index);
extern void arch_register_drivers();
extern ulong arch_get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2);
extern void arch_set_syscall_return(struct reg_context *regs, int succeed, ulong return0, ulong return1);
extern int arch_handle_syscall(ulong num, ulong param0, ulong param1, ulong param2, ulong *return0, ulong *return1);
extern void arch_disable_local_int();
extern void arch_enable_local_int();
extern void arch_switch_to(ulong sched_id, struct reg_context *context,
                           ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb);
extern void arch_kernel_dispatch_prep(ulong sched_id, struct kernel_dispatch_info *kdi);

extern void hal_entry_primary(struct loader_args *largs, struct hal_arch_funcs *funcs);
extern void hal_entry_secondary();



#endif