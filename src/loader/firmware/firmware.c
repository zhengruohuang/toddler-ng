#include "common/include/inttypes.h"
#include "loader/include/loader.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"


void init_firmware()
{
    struct firmware_args *fw_args = get_fw_args();

    switch (fw_args->type) {
    case FW_NONE:
        init_appended_fdt();
        break;
    case FW_FDT:
        init_supplied_fdt(fw_args->fdt.fdt);
        break;
    case FW_KARG:
        init_appended_fdt();
        init_karg(fw_args->karg.kargc, fw_args->karg.kargv, fw_args->karg.env);
        break;
    case FW_ATAGS:
        init_appended_fdt();
        init_atags(fw_args->atags.atags);
        break;
    default:
        panic("Unable to init firmware!\n");
        break;
    }
}

void *firmware_translate_virt_to_phys(void *vaddr)
{
    struct firmware_args *fw_args = get_fw_args();

    if (fw_args->type == FW_OFW) {
        // FIXME
        panic("Not implemented!");
        return vaddr;
    }

    struct loader_arch_funcs *arch_funcs = get_arch_funcs();
    if (arch_funcs->access_win_to_phys) {
        return arch_funcs->access_win_to_phys(vaddr);
    }

    return vaddr;
}

void *firmware_alloc_and_map_acc_win(void *paddr, ulong size, ulong align)
{
    struct firmware_args *fw_args = get_fw_args();

    if (fw_args->type == FW_OFW) {
        // FIXME
        panic("Not implemented!");
        return paddr;
    }

    struct loader_arch_funcs *arch_funcs = get_arch_funcs();
    if (arch_funcs->phys_to_access_win) {
        return arch_funcs->phys_to_access_win(paddr);
    }

    return paddr;
}
