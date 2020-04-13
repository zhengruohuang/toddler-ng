#ifndef __LOADER_INCLUDE_FIRMWARE_H__
#define __LOADER_INCLUDE_FIRMWARE_H__


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "loader/include/loader.h"


/*
 * Firmware driver
 */
struct firmware_driver {
    const char *name;
    struct firmware_driver *next;

    int need_fdt;

    void (*init)(void *param);
    void (*add_initrd)(void *initrd_start, ulong initrd_size);
    paddr_t (*translate_vaddr_to_paddr)(ulong vaddr);
    void *(*alloc_and_map_acc_win)(paddr_t paddr, ulong size, ulong align);
};

#define DECLARE_FIRMWARE_DRIVER(name)                       \
    struct firmware_driver __##name##_fw_driver

#define REGISTER_FIRMWARE_DRIVER(name)                      \
    extern struct firmware_driver __##name##_fw_driver;     \
    register_firmware_driver(&__##name##_fw_driver)


/*
 * General
 */
extern void init_firmware();
extern paddr_t firmware_translate_virt_to_phys(ulong vaddr);
extern void *firmware_alloc_and_map_acc_win(paddr_t paddr, ulong size, ulong align);


/*
 * Bootargs
 */
extern char *bootargs_parse_data(const char *key, const char *subkey);
extern u64 bootargs_parse_u64(const char *key, const char *subkey, const int base, u64 def);
extern u32 bootargs_parse_u32(const char *key, const char *subkey, const int base, u32 def);
extern ulong bootargs_parse_ulong(const char *key, const char *subkey, const int base, ulong def);
extern int bootargs_parse_int(const char *key, const char *subkey, int def);
extern int bootargs_parse_char(const char *key, const char *subkey, int def);
extern int bootargs_parse_str(const char *key, const char *subkey, char *buf, size_t len);
extern int bootargs_parse_switch(const char *key, const char *subkey, int set, int nset, int def);
extern int bootargs_parse_bool(const char *key, const char *subkey);
extern void init_bootargs();


/*
 * MIPS Karg
 */
struct firmware_params_karg {
    int kargc;
    char **kargv;
    char **env;
    u64 mem_size;
};


/*
 * ARM ATAGS
 */


/*
 * FDT
 */
extern int is_fdt_header(void *fdt);
extern int copy_fdt(void *buf, void *src, size_t size);
extern void init_supplied_fdt(void *supplied_fdt);
extern void init_appended_fdt();


/*
 * PPC/SPARC OFW
 */
struct firmware_params_ofw {
    void *entry;
    void *initrd_start;
    ulong initrd_size;
};


/*
 * SPARC OBP
 */


/*
 * PPC FDT
 */
struct firmware_params_ppc_fdt {
    void *fdt;
    u32 epapr_magic;
    u32 mapsize;
};


/*
 * PPC/m68k board info
 */
struct firmware_params_board_info {
    void *board_info;
    void *initrd_start, *initrd_end;
    void *cmd_start, *cmd_end;
};


/*
 * Alpha SRM
 */
struct firmware_params_srm {
    void *hwrpb_base;
    void *cmdline;
    void *initrd_start;
    ulong initrd_size;
};


/*
 * x86 Multiboot
 */


#endif
