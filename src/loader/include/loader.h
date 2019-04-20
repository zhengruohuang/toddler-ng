#ifndef __LOADER_INCLUDE_LOADER_H__
#define __LOADER_INCLUDE_LOADER_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


enum firmware_types {
    FW_UNKNOWN = 0,
    FW_NONE,
    FW_FDT,
    FW_OBP,
    FW_OFW,
    FW_ATAGS,
    FW_MULTIBOOT,
    FW_KARG,
    FW_PPC_FDT,
    FW_BOARD,
    FW_SH_HDR,
};

struct firmware_args {
    // Common
    void *arch_args;
    enum firmware_types type;

    union {
        // General FDT
        struct {
            void *fdt;
        } fdt;

        // MIPS karg
        struct {
            int kargc;
            char **kargv;
            char **env;
            u64 mem_size;
        } karg;

        // ARM atags
        struct {
            void *atags;
        } atags;

        // x86
        struct {
            void *multiboot;
        } multiboot;

        // SPARC OBP
        struct {
            void *obp;
        } obp;

        // PPC FDT
        struct {
            void *fdt;
            u32 epapr_magic;
            u32 mapsize;
        } ppc_fdt;

        // PPC/SPARC OFW
        struct {
            void *ofw;
            void *initrd_start;
            u32 initrd_size;
        } ofw;

        // PPC/m68k board info
        struct {
            void *board_info;
            void *initrd_start, *initrd_end;
            void *cmd_start, *cmd_end;
        } board;
    };
};

struct loader_arch_funcs {
    // Per-arch info
    ulong reserved_stack_size;
    ulong page_size;
    int num_reserved_got_entries;
    u64 phys_mem_range_min, phys_mem_range_max;

    // General
    void (*init_arch)();
    void (*jump_to_hal)();

    // Paging
    // Map range returns pages mapped
    void *(*setup_page)();
    int (*map_range)(void *page_table, void *vaddr, void *paddr, ulong size,
        int cache, int exec, int write);

    // Access window <--> physical addr
    // Probably only needed by MIPS
    void *(*access_win_to_phys)(void *vaddr);
    void *(*phys_to_access_win)(void *paddr);
};

struct loader_args {
    // Arch-specific data
    void *arch_args;

    // Important data structures
    void *devtree;
    void *memmap;
    void *page_table;

    // HAL virtual layout, hal_grow: >=0 = grow up, <0 = grow down
    void *hal_entry, *hal_start, *hal_end;
    int hal_grow;

    // Kernel virtual layout
    void *kernel_entry, *kernel_start, *kernel_end;
};


extern struct firmware_args *get_fw_args();
extern struct loader_arch_funcs *get_arch_funcs();
extern struct loader_args *get_loader_args();

extern void loader(struct firmware_args *args,
    struct loader_arch_funcs *funcs);


#endif
