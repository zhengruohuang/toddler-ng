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
    // Allocate physical pages, at any location
    ppfn_t (*palloc)(int count);
    int (*pfree)(ppfn_t ppfn);

    // Allocate direct mapped physical pages and covert to accessible pointer
    void *(*palloc_ptr)(int count);
    int (*pfree_ptr)(void *ptr);

    // Kernel dispatch
    void (*dispatch)(ulong sched_id, struct kernel_dispatch *kdi);

    // Start working
    void (*start)();

    // Test kernel components
    void (*test_phase1)();
};


/*
 * HAL exported variables and functions
 */
// General
typedef int (*putchar_t)(int s);
typedef void (*halt_t)(int count, ...);

// Direct access
typedef ulong (*direct_paddr_to_vaddr_t)(paddr_t paddr, int count, int cached);
typedef paddr_t (*direct_vaddr_to_paddr_t)(ulong vaddr, int count);

// MP
typedef ulong (*get_cur_mp_id_t)();
typedef int (*get_cur_mp_seq_t)();
typedef void *(*access_per_cpu_var_t)(int *offset, size_t size);

// Interrupts
typedef int (*disable_local_int_t)();
typedef void (*enable_local_int_t)();

// User-space interrupt handler
typedef void *(*int_register_t)(ulong fw_id, int fw_pos, ulong user_seq);
typedef void *(*int_register2_t)(const char *fw_path, int fw_pos, ulong user_seq);
typedef void (*int_eoi_t)(void *hal_dev);

// Clock
typedef u64 (*get_ms_t)();

// Mapping
typedef int (*map_range_t)(void *page_table, ulong vaddr, paddr_t paddr,
                           size_t length, int cacheable, int exec, int write,
                           int kernel, int override);
typedef int (*unmap_range_t)(void *page_table, ulong vaddr, paddr_t paddr,
                             size_t length);
typedef paddr_t (*page_translate_t)(void *page_table, ulong vaddr);

// Address space
typedef void *(*init_addr_space_t)();
typedef void (*free_addr_space_t)(void *ptr);

// Context
typedef void (*init_context_t)(struct reg_context *context, ulong entry, ulong param,
                               ulong stack_top, ulong tcb, int user_mode);
typedef void (*set_context_param_t)(struct reg_context *context, ulong param);
typedef void (*switch_context_t)(ulong thread_id, struct reg_context *context,
                                 void *page_table, int user_mode, ulong asid, ulong tcb);

// Syscall
typedef void (*set_syscall_return_t)(struct reg_context *context,
                                     int success, ulong return0, ulong return1);

// TLB
typedef void (*invalidate_tlb_t)(ulong asid, ulong vaddr, size_t length);
typedef void (*flush_tlb_t)();


struct hal_exports {
    // Kernel exports, this field is set by kernel
    struct kernel_exports *kernel;

    // General functions
    putchar_t putchar;
    void (*time)(ulong *high, ulong *low);
    void (*idle)();
    void (*halt)();

    // Indicates if direct physical address access is possible
    int has_direct_access;
    direct_paddr_to_vaddr_t direct_paddr_to_vaddr;
    direct_vaddr_to_paddr_t direct_vaddr_to_paddr;

    // Kernel info
    void *kernel_page_table;

    // Device tree
    void *devtree;

    // Core image
    void *coreimg;

    // MP
    int num_cpus;
    get_cur_mp_id_t get_cur_mp_id;
    get_cur_mp_seq_t get_cur_mp_seq;
    access_per_cpu_var_t access_per_cpu_var;

    // Physical memory info
    int memmap_count;
    struct loader_memmap *memmap;

    // Interrupts
    disable_local_int_t disable_local_int;
    enable_local_int_t enable_local_int;
    int (*restore_local_int)(int enabled);

    // User-space interrupt handler
    int_register_t int_register;
    int_register2_t int_register2;
    int_eoi_t int_eoi;

    // Clock
    get_ms_t get_ms;

    // Mapping
    map_range_t map_range;
    unmap_range_t unmap_range;
    page_translate_t translate;

    // Address space
    ulong vaddr_base, vaddr_limit;
    ulong asid_limit;
    init_addr_space_t init_addr_space;
    free_addr_space_t free_addr_space;

    // Context
    init_context_t init_context;
    set_context_param_t set_context_param;
    switch_context_t switch_context;
    set_syscall_return_t set_syscall_return;

    // TLB
    invalidate_tlb_t invalidate_tlb;
    flush_tlb_t flush_tlb;
};


/*
 * Kernel entry
 */
typedef void (*kernel_entry_t)(struct hal_exports *exp);


#endif
