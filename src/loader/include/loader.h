#ifndef __LOADER_INCLUDE_LOADER_H__
#define __LOADER_INCLUDE_LOADER_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "loader/include/export.h"


struct firmware_args {
    void *arch_args;

    char *fw_name;
    void *fw_params;
};

struct loader_arch_funcs {
    // Per-arch info
    ulong reserved_stack_size;
    ulong page_size;
    int num_reserved_got_entries;
    u64 phys_mem_range_min, phys_mem_range_max;
    ulong mp_entry;

    // General
    void (*init_libk)();
    void (*init_arch)();
    void (*final_memmap)();
    void (*final_arch)();
    void (*jump_to_hal)();

    // Paging
    // Map range returns pages mapped
    void *(*setup_page)();
    int (*map_range)(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
        int cache, int exec, int write);

    // Access window <--> physical addr
    // Probably only needed by MIPS
    paddr_t (*access_win_to_phys)(void *ptr);
    void *(*phys_to_access_win)(paddr_t paddr);
};


extern struct firmware_args *get_fw_args();
extern struct loader_arch_funcs *get_loader_arch_funcs();
extern struct loader_args *get_loader_args();

extern void loader(struct firmware_args *args,
    struct loader_arch_funcs *funcs);


#endif
