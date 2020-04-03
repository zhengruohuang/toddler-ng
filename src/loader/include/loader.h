#ifndef __LOADER_INCLUDE_LOADER_H__
#define __LOADER_INCLUDE_LOADER_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "loader/include/export.h"


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
    FW_SRM,
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

        // Alpha SRM
        struct {
            char *cmdline;
            void *initrd_start;
            ulong initrd_size;
            void *hwrpb_base;
        } srm;
    };
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
    void (*final_arch)();
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


extern struct firmware_args *get_fw_args();
extern struct loader_arch_funcs *get_loader_arch_funcs();
extern struct loader_args *get_loader_args();

extern void loader(struct firmware_args *args,
    struct loader_arch_funcs *funcs);


#endif
