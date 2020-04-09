#include "common/include/inttypes.h"
#include "loader/include/loader.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/devtree.h"
#include "loader/include/firmware.h"


void init_firmware()
{
    struct firmware_args *fw_args = get_fw_args();
    init_devtree();

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
    case FW_OFW:
        init_ofw(fw_args->ofw.ofw);
        ofw_add_initrd(fw_args->ofw.initrd_start, fw_args->ofw.initrd_size);
        break;
    case FW_OBP:
        init_obp(fw_args->obp.obp);
        break;
    case FW_SRM:
        init_srm(fw_args->srm.hwrpb_base);
        srm_add_initrd(fw_args->srm.initrd_start, fw_args->srm.initrd_size);
        break;
    case FW_MULTIBOOT:
        init_multiboot(fw_args->multiboot.multiboot);
        break;
    default:
        panic("Unable to init firmware!\n");
        break;
    }

    struct loader_args *largs = get_loader_args();
    largs->devtree = devtree_get_head();
}

paddr_t firmware_translate_virt_to_phys(ulong vaddr)
{
    struct firmware_args *fw_args = get_fw_args();

    if (fw_args->type == FW_OFW) {
        return ofw_translate_virt_to_phys(vaddr);
    } else if (fw_args->type == FW_OBP) {
        return obp_translate_virt_to_phys(vaddr);
    }

    struct loader_arch_funcs *arch_funcs = get_loader_arch_funcs();
    if (arch_funcs->access_win_to_phys) {
        return arch_funcs->access_win_to_phys((void *)vaddr);
    }

    return cast_vaddr_to_paddr(vaddr);
}

void *firmware_alloc_and_map_acc_win(paddr_t paddr, ulong size, ulong align)
{
    /*
     * This function allocates vaddr, and maps it to the given paddr
     * Then returns vaddr
     */

    struct firmware_args *fw_args = get_fw_args();

    if (fw_args->type == FW_OFW) {
        return ofw_alloc_and_map_acc_win(paddr, size, align);
    } else if (fw_args->type == FW_OBP) {
        return obp_alloc_and_map_acc_win(paddr, size, align);
    }

    /*
     * On some systems, before MMU is enabled or in some memory regions,
     * there is a fixed mapping between vaddr and paddr, e.g. MIPS kseg0 and kseg1
     * Thus we simply do an arch-specific translation
     */

    struct loader_arch_funcs *arch_funcs = get_loader_arch_funcs();
    if (arch_funcs->phys_to_access_win) {
        return arch_funcs->phys_to_access_win(paddr);
    }

    /*
     * Finally, on some systems, we directly work with physical memory,
     * and thus vaddr allocation or translation is not needed
     */

    return cast_paddr_to_ptr(paddr);
}
