#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "common/include/io.h"
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
 * Raspi2 UART
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)
#define PL011_DR                (PL011_BASE)
#define PL011_FR                (PL011_BASE + 0x18ul)

static int raspi2_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    u32 ready = 0;
    while (!ready) {
        ready = !(mmio_read8(PL011_FR) & 0x20);
    }

    // Write the character to the FIFO for transmission
    mmio_write8(PL011_DR, ch & 0xff);

    return 1;
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(raspi2_putchar);
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
}

static void init_int_mp()
{
    init_int_entry_mp();
}

static void init_mm()
{
}

static void init_mm_mp()
{
}


/*
 * Simple and quick funcs
 */
static void disable_local_int()
{
    __asm__ __volatile__ (
        "cpsid aif;"
        :
        :
        : "memory"
    );
}

static void enable_local_int()
{
    //kprintf("enable_local_int\n");

    __asm__ __volatile__ (
        "cpsie aif;"
        :
        :
        : "memory"
    );
}

static ulong get_cur_mp_id()
{
    struct mp_affinity_reg mpidr;
    read_cpu_id(mpidr.value);
    return mpidr.lo24;
}

static void register_drivers()
{
    REGISTER_DEV_DRIVER(bcm2835_armctrl_intc);
    REGISTER_DEV_DRIVER(bcm2836_percpu_intc);

    REGISTER_DEV_DRIVER(armv7_generic_timer);

    REGISTER_DEV_DRIVER(armv7_cpu);

    REGISTER_DEV_DRIVER(arm_pl011);
}

static ulong get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    *param0 = regs->r1;
    *param1 = regs->r2;
    *param2 = regs->r3;

    return regs->r0;
}

static void set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
    regs->r0 = success;
    regs->r1 = return0;
    regs->r2 = return1;
}

static void halt_cur_cpu(int count, va_list args)
{
    while (1);
}

static void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
    atomic_mb();
    inv_tlb_all();
    atomic_ib();
}

static void flush_tlb()
{
    atomic_mb();
    inv_tlb_all();
    atomic_ib();
}

static void start_cpu(int mp_seq, ulong mp_id, ulong entry)
{
}

static void *init_user_page_table()
{
    struct l1table *page_table = kernel_palloc_ptr(L1PAGE_TABLE_SIZE / PAGE_SIZE);
    memzero(page_table, L1PAGE_TABLE_SIZE);

    struct loader_args *largs = get_loader_args();
    struct l1table *kernel_page_table = largs->page_table;

    // Duplicate the last 4MB mapping
    page_table->value_u32[4095] = kernel_page_table->value_u32[4095];
    page_table->value_u32[4094] = kernel_page_table->value_u32[4094];
    page_table->value_u32[4093] = kernel_page_table->value_u32[4093];
    page_table->value_u32[4092] = kernel_page_table->value_u32[4092];

    return page_table;
}

static void free_user_page_table(void *ptr)
{
    kernel_pfree_ptr(ptr);
}

static void set_thread_context_param(struct reg_context *context, ulong param)
{
    context->r0 = param;
}

static void init_thread_context(struct reg_context *context, ulong entry,
                                ulong param, ulong stack_top, int user_mode)
{
    // Set GPRs
    memzero(context, sizeof(struct reg_context));

    // Set param
    set_thread_context_param(context, param);
    //context->r0 = param;

    // Set PC and SP
    context->sp = stack_top;
    context->pc = entry;

    // Set CPSR
    struct proc_status_reg psr;
    psr.value = 0;
    psr.mode = user_mode ? 0x10 : 0x1f;
    context->cpsr = psr.value;
}


/*
 * ARM HAL entry
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

    funcs.putchar = raspi2_putchar;
    funcs.halt = halt_cur_cpu;

    funcs.has_direct_access = 0;
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

    funcs.vaddr_limit = USER_VADDR_LIMIT;
    funcs.asid_limit = 0; // TODO: ASID requires PAE and 64-bit TTBRs
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
    // Switch to CPU-private stack, but leave some space for interrupt handling
    ulong sp = get_my_cpu_init_stack_top_vaddr();
    ulong pc = (ulong)&hal_mp;

    __asm__ __volatile__ (
        // Set up stack top
        "mov sp, %[sp];"

        // Jump to target
        "mov pc, %[pc];"
        :
        : [pc] "r" (pc), [sp] "r" (sp)
        : "memory"
    );
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
