#include "loader/include/lib.h"
#include "loader/include/loader.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/mem.h"
#include "loader/include/devtree.h"
#include "loader/include/kprintf.h"


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

static void save_extra_loader_args()
{
    // Save loader stack limits
    loader_args.stack_limit = arch_funcs ? arch_funcs->stack_limit : 0;
    loader_args.stack_limit_mp = arch_funcs ? arch_funcs->stack_limit_mp : 0;

    // Save MP entry
    loader_args.mp_entry = arch_funcs ? arch_funcs->mp_entry : 0;

    // Save memory map
    loader_args.memmap = get_memmap();
    print_memmap();
}


/*
 * Stack protect
 */
static void init_stack_prot()
{
    if (arch_funcs && arch_funcs->stack_limit) {
        set_stack_magic(arch_funcs->stack_limit);
    }

    if (arch_funcs && arch_funcs->stack_limit_mp) {
        set_stack_magic(arch_funcs->stack_limit_mp);
    }
}

static void check_stack()
{
    if (arch_funcs && arch_funcs->stack_limit) {
        check_stack_magic(arch_funcs->stack_limit);
        check_stack_pos(arch_funcs->stack_limit);
    }
}

static void check_stack_mp()
{
    if (arch_funcs && arch_funcs->stack_limit_mp) {
        check_stack_magic(arch_funcs->stack_limit_mp);
        check_stack_pos(arch_funcs->stack_limit_mp);
    }
}


/*
 * Wrappers
 */
static void init_libk()
{
    if (arch_funcs && arch_funcs->init_libk) {
        arch_funcs->init_libk();
    }
    kprintf("In Loader!\n");
}

static void init_arch()
{
    if (arch_funcs && arch_funcs->init_arch) {
        arch_funcs->init_arch();
    }
}

static void init_arch_mp()
{
    if (arch_funcs && arch_funcs->init_arch_mp) {
        arch_funcs->init_arch_mp();
    }
}

static void final_memmap()
{
    if (arch_funcs && arch_funcs->final_memmap) {
        arch_funcs->final_memmap();
    }
}

static void final_arch()
{
    if (arch_funcs && arch_funcs->final_arch) {
        arch_funcs->final_arch();
    }
}

static void final_arch_mp()
{
    if (arch_funcs && arch_funcs->final_arch_mp) {
        arch_funcs->final_arch_mp();
    }
}

static void jump_to_hal()
{
    panic_if(!arch_funcs || !arch_funcs->jump_to_hal,
        "Arch loader must implement jump_to_hal()!");

    arch_funcs->jump_to_hal();
}

static void jump_to_hal_mp()
{
    panic_if(!arch_funcs || !arch_funcs->jump_to_hal_mp,
        "Arch loader must implement jump_to_hal_mp()!");

    arch_funcs->jump_to_hal_mp();
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
    init_arch();
    init_libk();

    // Stack protect
    init_stack_prot();

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
    final_memmap();

    // Paging
    init_page();

    // Load HAL and kernel
    load_hal_and_kernel();

    // Finalize
    final_arch();

    // Save extra loader args
    save_extra_loader_args();

    // Jump to HAL
    check_stack();
    jump_to_hal();

    // Done
    devtree_print(NULL);

    kprintf("Loader done!\n");
    unreachable();
}

void loader_mp()
{
    // Init
    init_arch_mp();

    // Finalize
    final_arch_mp();

    // Jump to HAL
    check_stack_mp();
    jump_to_hal_mp();

    kprintf("MP Loader done!\n");
    unreachable();
}
