#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "common/include/io.h"
#include "hal/include/hal.h"
#include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/mp.h"
#include "hal/include/driver.h"
#include "hal/include/kernel.h"


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


/*
 * Per-arch init
 */
static void init_libk()
{
    init_libk_putchar(malta_putchar);
}

static void init_arch()
{
}

static void init_arch_mp()
{
}

static void init_int()
{
    init_int_entry();
    init_switch();
}

static void init_int_mp()
{
    init_int_entry_mp();
    init_switch_mp();
}

static void init_mm()
{
    init_tlb();
}

static void init_mm_mp()
{
    init_tlb_mp();
}


/*
 * Direct access
 */
void *cast_paddr_to_cached_seg(paddr_t paddr)
{
    paddr_t cached_seg = paddr | (paddr_t)SEG_DIRECT_CACHED;
    return cast_paddr_to_ptr(cached_seg);
}

ulong cast_paddr_to_uncached_seg(paddr_t paddr)
{
    paddr_t cached_seg = paddr | (paddr_t)SEG_DIRECT_UNCACHED;
    return cast_paddr_to_vaddr(cached_seg);
}

paddr_t cast_cached_seg_to_paddr(void *ptr)
{
    ulong vaddr = (ulong)ptr;
    ulong lower = vaddr & (ulong)SEG_DIRECT_MASK;
    return cast_vaddr_to_paddr(lower);
}

paddr_t cast_uncached_seg_to_paddr(ulong vaddr)
{
    ulong lower = vaddr & (ulong)SEG_DIRECT_MASK;
    return cast_vaddr_to_paddr(lower);
}

static ulong hal_direct_paddr_to_vaddr(paddr_t paddr, int count, int cached)
{
    paddr |= (paddr_t)(cached ? SEG_DIRECT_CACHED : SEG_DIRECT_UNCACHED);
    return cast_paddr_to_vaddr(paddr);
}

static paddr_t hal_direct_vaddr_to_paddr(ulong vaddr, int count)
{
    ulong lower = vaddr & (ulong)SEG_DIRECT_MASK;
    return cast_vaddr_to_paddr(lower);
}


/*
 * Simple and quick funcs
 */
static void disable_local_int()
{
    struct cp0_status sr;

    read_cp0_status(sr.value);
    sr.ie = 0;
    write_cp0_status(sr.value);
}

static void enable_local_int()
{
    struct cp0_status sr;

    read_cp0_status(sr.value);
    sr.ie = 1;
    write_cp0_status(sr.value);
}

static ulong get_cur_mp_id()
{
    u32 cpu_num = 0;
    struct cp0_ebase ebase;
    read_cp0_ebase(ebase.value);

    cpu_num = ebase.cpunum;
    return (ulong)cpu_num;
}

static void register_drivers()
{
    REGISTER_DEV_DRIVER(mips_cpu_timer);
}

static ulong get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    *param0 = regs->a0;
    *param1 = regs->a1;
    *param2 = regs->a2;

    return regs->v0;
}

static void set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
    regs->v0 = success;
    regs->a0 = return0;
    regs->a1 = return1;
}

static void halt_cur_cpu(int count, va_list args)
{
    disable_local_int();
    while (1) {
        __asm__ __volatile__ ("wait;");
    }
}

static void start_cpu(int mp_seq, ulong mp_id, ulong entry)
{
}

static void set_thread_context_param(struct reg_context *regs, ulong param)
{
    regs->a0 = param;
}

static void init_thread_context(struct reg_context *regs, ulong entry,
                                ulong param, ulong stack_top, int user_mode)
{
    // Set GPRs
    memzero(regs, sizeof(struct reg_context));

    // Set param
    regs->a0 = param;

    // Set other states
    regs->sp = stack_top;
    regs->pc = entry;
    regs->delay_slot = 0;

    //if (!user_mode) {
    //    context->sp |= SEG_CACHED;
    //}
}


/*
 * MIPS HAL entry
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

    funcs.putchar = malta_putchar;
    funcs.halt = halt_cur_cpu;

    funcs.has_direct_access = 1;
    funcs.direct_paddr_to_vaddr = hal_direct_paddr_to_vaddr;
    funcs.direct_vaddr_to_paddr = hal_direct_vaddr_to_paddr;
    funcs.map_range = map_range;
    funcs.unmap_range = unmap_range;
    funcs.translate = translate;

    funcs.get_cur_mp_id = get_cur_mp_id;
    funcs.mp_entry = largs->mp_entry;
    funcs.start_cpu = start_cpu;

    funcs.register_drivers = register_drivers;

    funcs.get_syscall_params = get_syscall_params;
    funcs.set_syscall_return = set_syscall_return;
    funcs.handle_arch_syscall = NULL;

    funcs.arch_disable_local_int = disable_local_int;
    funcs.arch_enable_local_int = enable_local_int;

    funcs.init_addr_space = init_user_page_table;
    funcs.free_addr_space = free_user_page_table;
    funcs.init_context = init_thread_context;
    funcs.set_context_param = set_thread_context_param;
    funcs.switch_to = switch_to;
    funcs.kernel_pre_dispatch = kernel_pre_dispatch;
    funcs.kernel_post_dispatch = kernel_post_dispatch;

    funcs.invalidate_tlb = invalidate_tlb;

    hal(largs, &funcs);
}

void hal_entry(struct loader_args *largs, int mp)
{
    if (mp) {
        //hal_entry_mp();
    } else {
        hal_entry_bsp(largs);
    }

    // Should never reach here
    panic("Should never reach here");
    while (1);
}
