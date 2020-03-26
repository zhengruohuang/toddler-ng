#include "common/include/io.h"
#include "hal/include/hal.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"


/*
 * Malta UART
 */
#define SOUTH_BRIDGE_BASE_ADDR  0x18000000
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x28)

static int malta_putchar(int ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;
    }

    mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


static void init_libk()
{
    init_libk_putchar(malta_putchar);
}

static void init_arch()
{
}


/*
 * MIPS HAL entry
 */
void hal_entry(struct loader_args *largs)
{
    struct hal_arch_funcs funcs;
    memzero(&funcs, sizeof(struct hal_arch_funcs));

    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;

    hal_entry_primary(largs, &funcs);
}

