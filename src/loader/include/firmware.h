#ifndef __LOADER_INCLUDE_FIRMWARE_H__
#define __LOADER_INCLUDE_FIRMWARE_H__


#include "common/include/inttypes.h"
#include "loader/include/loader.h"


/*
 * General
 */
extern void init_firmware();
extern void *firmware_translate_virt_to_phys(void *vaddr);
extern void *firmware_alloc_and_map_acc_win(void *paddr, ulong size, ulong align);


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
 * Karg
 */
extern void init_karg(int fw_kargc, char **fw_kargv, char **fw_env);


/*
 * ATAGS
 */
extern void init_atags(void *atags);


/*
 * FDT
 */
extern int is_fdt_header(void *fdt);
extern void init_supplied_fdt(void *supplied_fdt);
extern void init_appended_fdt();


/*
 * OFW
 */
extern void init_ofw(void *entry);
extern void ofw_add_initrd(void *initrd_start, ulong initrd_size);
extern void *ofw_translate_virt_to_phys(void *vaddr);
extern void *ofw_alloc_and_map_acc_win(void *paddr, ulong size, ulong align);


/*
 * OBP
 */
extern void init_obp(void *entry);
extern void *obp_translate_virt_to_phys(void *vaddr);
extern void *obp_alloc_and_map_acc_win(void *paddr, ulong size, ulong align);


/*
 * SRM
 */
extern void init_srm(void *hwrpb_base);
extern void srm_add_initrd(void *initrd_start, ulong initrd_size);


/*
 * Multiboot
 */
extern void init_multiboot(void *multiboot);


#endif
