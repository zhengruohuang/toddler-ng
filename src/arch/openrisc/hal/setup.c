#include "common/include/inttypes.h"
#include "common/include/msr.h"
// #include "common/include/io.h"
#include "hal/include/hal.h"
// #include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/driver.h"


/*
 * OpenRISC UART
 */
static inline void mmio_mb()
{
//     atomic_mb();
}

static inline void mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(addr);
    *ptr = val;
    mmio_mb();
}

#define UART_BASE_ADDR          (0x90000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)

static int or1k_putchar(int ch)
{
    mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(or1k_putchar);
}

static void init_arch()
{
}

static void init_arch_mp()
{
}

static void init_int()
{
//     init_int_entry();
}

static void init_int_mp()
{
//     init_int_entry_mp();
}

static void init_mm()
{
}

static void init_mm_mp()
{
}


/*
 * OpenRISC HAL entry
 */
static void hal_entry_bsp(struct loader_args *largs)
{
    struct hal_arch_funcs funcs;
    memzero(&funcs, sizeof(struct hal_arch_funcs));

    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.init_arch_mp = init_arch_mp;
    funcs.init_int = init_int;
    funcs.init_int_mp = init_int_mp;
    funcs.init_mm = init_mm;
    funcs.init_mm_mp = init_mm_mp;

    funcs.putchar = or1k_putchar;
//     funcs.halt = halt_cur_cpu;
//
//     funcs.has_direct_access = 0;
//     funcs.hal_direct_access = NULL;
//     funcs.map_range = map_range;
//     funcs.unmap_range = unumap_range;
//     funcs.translate = translate;
//
//     funcs.get_cur_mp_id = get_cur_mp_id;
//     funcs.mp_entry = largs->mp_entry;
//     funcs.start_cpu = start_cpu;
//
//     funcs.register_drivers = register_drivers;
//
//     funcs.get_syscall_params = get_syscall_params;
//     funcs.set_syscall_return = set_syscall_return;
//     funcs.handle_arch_syscall = NULL;
//
//     funcs.arch_disable_local_int = disable_local_int;
//     funcs.arch_enable_local_int = enable_local_int;
//
//     funcs.init_addr_space = init_user_page_dir;
//     funcs.init_context = init_thread_context;
//     funcs.set_context_param = set_thread_context_param;
//     funcs.switch_to = switch_to;
//     funcs.kernel_dispatch_prep = NULL;
//
//     funcs.invalidate_tlb = invalidate_tlb;

    hal(largs, &funcs);
}

static void hal_entry_mp()
{
}

void hal_entry(struct loader_args *largs, int mp)
{
    if (mp) {
        hal_entry_mp();
    } else {
        hal_entry_bsp(largs);
    }

    // Should never reach here
    panic("Should never reach here");
    while (1);
}

