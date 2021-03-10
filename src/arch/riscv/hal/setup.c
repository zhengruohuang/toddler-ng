#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "common/include/page.h"
#include "hal/include/hal.h"
// #include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/mem.h"
#include "hal/include/driver.h"
#include "hal/include/kernel.h"


/*
 * RISC-V UART
 */
// Virt @ 0x10000000, SiFive-U @ 0x10013000
#define UART_BASE_ADDR          (0x10000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x28)

static inline void _raw_mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)addr;
    atomic_mb();
    *ptr = val;
    atomic_mb();
}

static int riscv_uart_putchar(int ch)
{
//     u32 ready = 0;
//     while (!ready) {
//         ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;
//     }

    _raw_mmio_write8(UART_DATA_ADDR, ch & 0xff);
    return 1;
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(riscv_uart_putchar);
    init_libk_page_table(pre_palloc, NULL, NULL);
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

static void init_kernel_pre()
{
}

static void init_kernel_post()
{
}


/*
 * Simple and quick funcs
 */
static void disable_local_int()
{
//     __asm__ __volatile__ (
//         "cpsid aif;"
//         :
//         :
//         : "memory"
//     );
}

static void enable_local_int()
{
//     __asm__ __volatile__ (
//         "cpsie aif;"
//         :
//         :
//         : "memory"
//     );
}

static ulong get_cur_mp_id()
{
//     struct mp_affinity_reg mpidr;
//     read_cpu_id(mpidr.value);
//     return mpidr.lo24;
    return 0;
}

static void register_drivers()
{
//     REGISTER_DEV_DRIVER(bcm2835_armctrl_intc);
//     REGISTER_DEV_DRIVER(bcm2836_percpu_intc);
//
//     REGISTER_DEV_DRIVER(armv7_generic_timer);
//
//     REGISTER_DEV_DRIVER(armv7_cpu);
//
//     REGISTER_DEV_DRIVER(arm_pl011);
}

static ulong get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    return 0;
//     *param0 = regs->r1;
//     *param1 = regs->r2;
//     *param2 = regs->r3;
//
//     return regs->r0;
}

static void set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
//     regs->r0 = success;
//     regs->r1 = return0;
//     regs->r2 = return1;
}

static void idle_cur_cpu()
{
    __asm__ __volatile__ ( "wfi;" : : : "memory" );
}

static void halt_cur_cpu()
{
    while (1) {
        disable_local_int();
        idle_cur_cpu();
    }
}

static void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
//     atomic_mb();
//     inv_tlb_all();
//     atomic_ib();
}

static void flush_tlb()
{
//     atomic_mb();
//     inv_tlb_all();
//     atomic_ib();
}

static void start_cpu(int mp_seq, ulong mp_id, ulong entry)
{
}

static void *init_user_page_table()
{
    int num_pages = page_get_num_table_pages(1);
    struct page_frame *page_table = kernel_palloc_ptr(num_pages);
    memzero(page_table, num_pages * PAGE_SIZE);

    struct loader_args *largs = get_loader_args();
    struct page_frame *kernel_page_table = largs->page_table;

    // Duplicate the last 4MB mapping
    page_table->entries[1023].value = kernel_page_table->entries[1023].value;

    return page_table;
}

static void free_user_page_table(void *ptr)
{
    kernel_pfree_ptr(ptr);
}

static void set_thread_context_param(struct reg_context *context, ulong param)
{
//     context->r0 = param;
}

static void init_thread_context(struct reg_context *context, ulong entry,
                                ulong param, ulong stack_top, int user_mode)
{
    // Set GPRs
    memzero(context, sizeof(struct reg_context));

    // Set param
    set_thread_context_param(context, param);

//     // Set PC and SP
//     context->sp = stack_top;
//     context->pc = entry;
//
//     // Set CPSR
//     struct proc_status_reg psr;
//     psr.value = 0;
//     psr.mode = user_mode ? 0x10 : 0x1f;
//     context->cpsr = psr.value;
}


/*
 * RISC-V HAL entry
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
    funcs.init_kernel_pre = init_kernel_pre;
    funcs.init_kernel_post = init_kernel_post;

    funcs.putchar = riscv_uart_putchar;
    funcs.idle = idle_cur_cpu;
    funcs.halt = halt_cur_cpu;

    funcs.has_direct_access = 0;
    funcs.hal_map_range = generic_map_range;
    funcs.kernel_map_range = generic_map_range;
    funcs.kernel_unmap_range = generic_unmap_range;
    funcs.translate = generic_translate;

    funcs.get_cur_mp_id = get_cur_mp_id;
    funcs.mp_entry = largs->mp_entry;
    funcs.start_cpu = start_cpu;

    funcs.register_drivers = register_drivers;

    funcs.get_syscall_params = get_syscall_params;
    funcs.set_syscall_return = set_syscall_return;
    funcs.handle_arch_syscall = NULL;

    funcs.arch_disable_local_int = disable_local_int;
    funcs.arch_enable_local_int = enable_local_int;

    funcs.vaddr_limit = USER_VADDR_LIMIT;
    funcs.asid_limit = 0; // TODO: check if ASID is supported
    funcs.init_addr_space = init_user_page_table;
    funcs.free_addr_space = free_user_page_table;

    funcs.init_context = init_thread_context;
    funcs.set_context_param = set_thread_context_param;
//     funcs.switch_to = switch_to;

//     funcs.kernel_pre_dispatch = kernel_pre_dispatch;
//     funcs.kernel_post_dispatch = kernel_post_dispatch;

    funcs.invalidate_tlb = invalidate_tlb;
    funcs.flush_tlb = flush_tlb;

    hal(largs, &funcs);
}

static void hal_entry_mp()
{
//     // Switch to CPU-private stack, but leave some space for interrupt handling
//     ulong sp = get_my_cpu_init_stack_top_vaddr();
//     ulong pc = (ulong)&hal_mp;
//
//     __asm__ __volatile__ (
//         // Set up stack top
//         "mov sp, %[sp];"
//
//         // Jump to target
//         "mov pc, %[pc];"
//         :
//         : [pc] "r" (pc), [sp] "r" (sp)
//         : "memory"
//     );
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
