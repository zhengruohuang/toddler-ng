#ifndef __HAL_INCLUDE_EXPORT__
#define __HAL_INCLUDE_EXPORT__


#include "common/include/inttypes.h"
#include "common/include/memmap.h"
#include "common/include/mem.h"
#include "hal/include/dispatch.h"
// #include "common/include/proc.h"


/*
 * Kernel exported variables and functions
 */
struct kernel_exports {
    ppfn_t (*palloc_tag)(int count, int tag);
    ppfn_t (*palloc)(int count);
    int (*pfree)(ppfn_t ppfn);
    void (*dispatch)(ulong sched_id, struct kernel_dispatch_info *kdi);
};


/*
 * HAL exported variables and functions
 */
// General
typedef int (*putchar_t)(int s);
typedef void (*halt_t)(int count, ...);

// MP
typedef ulong (*get_cur_mp_id_t)();
typedef int (*get_cur_mp_seq_t)();

// Interrupts
typedef int (*disable_local_int_t)();
typedef void (*enable_local_int_t)();

// Mapping
typedef int (*map_range_t)(void *page_table, ulong vaddr, paddr_t paddr,
                           size_t length, int cacheable, int exec, int write,
                           int kernel, int override);
typedef int (*unmap_range_t)(void *page_table, ulong vaddr, paddr_t paddr,
                             size_t length);
typedef ulong (*page_translate_t)(void *page_table, ulong vaddr);

// Address space
typedef void *(*init_addr_space_t)();

// Context
typedef void (*init_context_t)(struct reg_context *context, ulong entry,
                               ulong param, ulong stack_top, int user_mode);
typedef void (*set_context_param_t)(struct reg_context *context, ulong param);
typedef void (*switch_context_t)(ulong sched_id, struct reg_context *context,
                                 ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb);

// Syscall
typedef void (*set_syscall_return_t)(struct reg_context *context,
                                     int success, ulong return0, ulong return1);

// TLB
typedef void (*invalidate_tlb_t)(ulong asid, ulong vaddr, size_t length);


struct hal_exports {
    // Kernel exports, this field is set by kernel
    struct kernel_exports *kernel;

    // General functions
    putchar_t putchar;
    void (*time)(ulong *high, ulong *low);
    void (*halt)(int count, ...);

    // Kernel info
    void *kernel_page_table;

    // Core image
    ulong coreimg_load_addr;

    // MP
    int num_cpus;
    get_cur_mp_id_t get_cur_mp_id;
    get_cur_mp_seq_t get_cur_mp_seq;

    // Physical memory info
    int memmap_count;
    struct loader_memmap *memmap;

    // Interrupts
    disable_local_int_t disable_local_int;
    enable_local_int_t enable_local_int;
    int (*restore_local_int)(int enabled);

    // Mapping
    map_range_t map_range;
    unmap_range_t unmap_range;
    page_translate_t translate;

    // Address space
    int user_page_dir_page_count;
    ulong vaddr_space_end;
    init_addr_space_t init_addr_space;

    // Context
    init_context_t init_context;
    set_context_param_t set_context_param;
    switch_context_t switch_context;
    set_syscall_return_t set_syscall_return;

    // TLB
    invalidate_tlb_t invalidate_tlb;
};


#endif
