#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/msr.h"
#include "common/include/page.h"
#include "hal/include/hal.h"
#include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/mem.h"
#include "hal/include/driver.h"
#include "hal/include/kernel.h"


/*
 * OpenRISC UART
 */
static inline void _raw_mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(addr);
    *ptr = val;
    atomic_mb();
}

#define UART_BASE_ADDR          (0x90000000)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0)

static int or1k_putchar(int ch)
{
    _raw_mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


/*
 * Direct access
 */
static ulong hal_direct_paddr_to_vaddr(paddr_t paddr, int count, int cached)
{
    panic_if(!cached, "Uncached direct access not supported!\n");
    return cast_paddr_to_vaddr(paddr);
}

static paddr_t hal_direct_vaddr_to_paddr(ulong vaddr, int count)
{
    return cast_vaddr_to_paddr(vaddr);
}

static void *hal_direct_paddr_to_ptr(paddr_t paddr)
{
    return (void *)hal_direct_paddr_to_vaddr(paddr, 0, 1);
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(or1k_putchar);
    init_libk_page_table(pre_palloc, NULL, hal_direct_paddr_to_ptr);
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
    init_mmu();
}

static void init_int_mp()
{
    init_int_entry_mp();
    init_mmu_mp();
}

static void init_mm()
{
}

static void init_mm_mp()
{
}

static void init_kernel_pre()
{
    init_libk_page_table_alloc(NULL, NULL);
}

static void init_kernel_post()
{
    init_libk_page_table_alloc(kernel_palloc, kernel_pfree);
}


/*
 * Simple and quick funcs
 */
static void disable_local_int()
{
    struct supervision_reg sr;
    read_sr(sr.value);
    sr.timer_enabled = 0;
    sr.int_enabled = 0;
    write_sr(sr.value);
}

static void enable_local_int()
{
    struct supervision_reg sr;
    read_sr(sr.value);
    sr.timer_enabled = 1;
    sr.int_enabled = 1;
    write_sr(sr.value);
}

static ulong get_cur_mp_id()
{
    // TODO
    return 0;
}

static void register_drivers()
{
    REGISTER_DEV_DRIVER(or1k_intc);
    REGISTER_DEV_DRIVER(or1k_cpu_timer);
}

static ulong get_syscall_params(struct reg_context *regs, ulong *p1, ulong *p2, ulong *p3)
{
    *p1 = regs->a1;
    *p2 = regs->a2;
    *p3 = regs->a3;

    return regs->a0;
}

static void set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
    regs->v0 = success;
    regs->a1 = return0;
    regs->a2 = return1;
}

static void halt_cur_cpu(int count, va_list args)
{
    while (1) {
        __asm__ __volatile__ ( "" : : : "memory" );
    }
}

static void start_cpu(int mp_seq, ulong mp_id, ulong entry)
{
}

static void *init_user_page_table()
{
    int num_pages = page_get_num_table_pages(1);
    struct page_frame *page_table = kernel_palloc_ptr(num_pages);
    memzero(page_table, num_pages * PAGE_SIZE);

    struct page_frame *kernel_page_table = get_kernel_page_table();
    for (int i = 0; i < 4; i++) {
        page_table->entries[i].value = kernel_page_table->entries[i].value;
    }
    page_table->entries[1023].value = kernel_page_table->entries[1023].value;

    return page_table;
}

static void free_user_page_table(void *ptr)
{
    kernel_pfree_ptr(ptr);
}

static void set_thread_context_param(struct reg_context *context, ulong param)
{
    context->a0 = param;
}

static void init_thread_context(struct reg_context *context, ulong entry,
                                ulong param, ulong stack_top, int user_mode)
{
    // Set GPRs
    memzero(context, sizeof(struct reg_context));

    // Set param
    set_thread_context_param(context, param);

    // Set PC and SP
    context->sp = stack_top;
    context->pc = entry;

    // Set SR
    struct supervision_reg sr = { .value = 0 };
    sr.immu_enabled = 1;
    sr.dmmu_enabled = 1;
    sr.icache_enabled = 1;
    sr.dcache_enabled = 1;
    sr.int_enabled = 1;
    sr.timer_enabled = 1;
    sr.supervisor = !user_mode;
    context->sr = sr.value;
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
    funcs.init_kernel_pre = init_kernel_pre;
    funcs.init_kernel_post = init_kernel_post;

    funcs.putchar = or1k_putchar;
    funcs.halt = halt_cur_cpu;

    // OR1K HAL pretends to have direct access for cached accesses
    funcs.has_direct_access = 1;
    funcs.has_direct_access_uncached = 0;
    funcs.direct_paddr_to_vaddr = hal_direct_paddr_to_vaddr;
    funcs.direct_vaddr_to_paddr = hal_direct_vaddr_to_paddr;

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
    funcs.asid_limit = ASID_LIMIT;
    funcs.init_addr_space = init_user_page_table;
    funcs.free_addr_space = free_user_page_table;

    funcs.init_context = init_thread_context;
    funcs.set_context_param = set_thread_context_param;
    funcs.switch_to = switch_to;

    funcs.kernel_pre_dispatch = kernel_pre_dispatch;
    funcs.kernel_post_dispatch = kernel_post_dispatch;

    funcs.has_auto_tlb_flush_on_switch = 0;
    funcs.invalidate_tlb = invalidate_tlb;
    funcs.flush_tlb = flush_tlb;

    hal(largs, &funcs);
}

static void hal_entry_mp()
{
    panic("MP not implemented!\n");
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

