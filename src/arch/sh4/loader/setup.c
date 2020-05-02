#include "common/include/inttypes.h"
#include "loader/include/kprintf.h"
#include "loader/include/lib.h"
#include "loader/include/firmware.h"
#include "loader/include/boot.h"
#include "loader/include/mem.h"
#include "loader/include/loader.h"


/*
 * R2D UART
 */
#define UART_BASE_ADDR          (0xffe00000ul)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0xcul)

static void mmio_write8(unsigned long addr, unsigned char val)
{
    volatile unsigned char *ptr = (void *)addr;
    *ptr = val;
}

static int r2d_putchar(int ch)
{
    mmio_write8(UART_DATA_ADDR, ch & 0xff);
    return 1;
}

/*
 * Access window <--> physical addr
 */
static paddr_t access_win_to_phys(void *ptr)
{
    return cast_ptr_to_paddr(ptr);
}

static void *phys_to_access_win(paddr_t paddr)
{
    return cast_paddr_to_ptr(paddr);
}


/*
 * Paging
 */
static void *setup_page()
{
    return NULL;
}

static int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write)
{
    return 0;
}


/*
 * Jump to HAL
 */
typedef void (*hal_start_t)(struct loader_args *largs, int mp);

static void call_hal(struct loader_args *largs, int mp)
{
    hal_start_t hal = largs->hal_entry;
    hal(largs, 0);
}

static void jump_to_hal()
{
    struct loader_args *largs = get_loader_args();
    kprintf("Jump to HAL @ %p\n", largs->hal_entry);

    call_hal(largs, 0);
}


/*
 * Finalize
 */
static void final_memmap()
{
}

static void final_arch()
{
}


/*
 * Init arch
 */
static void init_arch()
{
}


/*
 * Init libk
 */
static void init_libk()
{
    init_libk_putchar(r2d_putchar);
}


/*
 * The dummy entry point
 */
void loader_entry()
{
    /*
     * BSS will be initialized at the beginning of loader() func
     * Thus no global vars are safe to access before init_arch() gets called
     * In other words, do not use global vars in loader_entry()
     */

    struct loader_arch_funcs funcs;
    struct firmware_args fw_args;

    // Safe to call lib funcs as they don't use any global vars
    memzero(&funcs, sizeof(struct loader_arch_funcs));
    memzero(&fw_args, sizeof(struct firmware_args));

    // Prepare arg
    fw_args.fw_name = "none";

    // Prepare funcs
    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.setup_page = setup_page;
    funcs.map_range = map_range;
    funcs.access_win_to_phys = access_win_to_phys;
    funcs.phys_to_access_win = phys_to_access_win;
    funcs.final_memmap = final_memmap;
    funcs.final_arch = final_arch;
    funcs.jump_to_hal = jump_to_hal;

    // Go to loader!
    loader(&fw_args, &funcs);
}
