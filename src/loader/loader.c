#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/devtree.h"
#include "loader/include/lprintf.h"


/*
 * BSS
 */
extern int __bss_start;
extern int __bss_end;

static void init_bss()
{
    int *cur;

    for (cur = &__bss_start; cur < &__bss_end; cur++) {
        *cur = 0;
    }
}


/*
 * Access arch and firmware funcs and info
 */
static struct firmware_args *fw_args = NULL;
static struct loader_arch_funcs *arch_funcs = NULL;
static struct loader_args loader_args;

struct firmware_args *get_fw_args()
{
    return fw_args;
}

struct loader_arch_funcs *get_loader_arch_funcs()
{
    return arch_funcs;
}

struct loader_args *get_loader_args()
{
    return &loader_args;
}


/*
 * Wrappers
 */
static void init_libk()
{
    if (arch_funcs && arch_funcs->init_libk) {
        arch_funcs->init_libk();
    }
}

static void init_arch()
{
    if (arch_funcs && arch_funcs->init_arch) {
        arch_funcs->init_arch();
    }
}

static void final_arch()
{
    if (arch_funcs && arch_funcs->final_arch) {
        arch_funcs->final_arch();
    }

    // Save MP entry
    loader_args.mp_entry = arch_funcs ? arch_funcs->mp_entry : 0;

    // Save memory map
    loader_args.memmap = get_memmap(&loader_args.num_memmap_entries,
                                    &loader_args.num_memmap_limit);
    print_memmap();
}

static void jump_to_hal()
{
    panic_if(!arch_funcs || !arch_funcs->jump_to_hal,
        "Arch loader must implement jump_to_hal()!");

    arch_funcs->jump_to_hal();
}


/*
 * The common loader entry
 */
void loader(struct firmware_args *args, struct loader_arch_funcs *funcs)
{
    // BSS
    init_bss();

    // Save fw args and arch funcs
    fw_args = args;
    arch_funcs = funcs;

    // Arch-specific
    init_libk();
    init_arch();

    // Firmware and device tree
    init_firmware();

    // Bootargs
    init_bootargs();

    // Periphs
    //firmware_create_periphs(fw_args);

    // Core image
    init_coreimg();

    // Memory map
    init_memmap();

    // Paging
    init_page();

    // Load HAL and kernel
    load_hal_and_kernel();

    // Finalize
    final_arch();

    // Jump to HAL
    jump_to_hal();

    // Done
    devtree_print(NULL);

    lprintf("Done!\n");
    while (1);
}
