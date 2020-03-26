#include "common/include/inttypes.h"
#include "common/include/msr.h"
#include "common/include/io.h"
#include "hal/include/hal.h"
#include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/dev.h"


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

static void init_int()
{
}

static void init_mm()
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
    __asm__ __volatile__ (
        "cpsie iaf;"
        :
        :
        : "memory"
    );
}

static int get_cur_cpu_index()
{
    u32 cpu_idx = 0;

    read_cpu_id(cpu_idx);
    cpu_idx &= 0x3;

    return (int)cpu_idx;
}

static void register_drivers()
{
    REGISTER_DEV_DRIVER(bcm2835_armctrl_intc);
    REGISTER_DEV_DRIVER(bcm2836_percpu_intc);

    REGISTER_DEV_DRIVER(armv7_generic_timer);
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

static void invalidate_tlb_line(ulong asid, ulong vaddr)
{
//     kprintf("TLB shootdown @ %lx, ASID: %lx ...", vaddr, asid);
    inv_tlb_all();
    // TODO: atomic_mb();
}

static void invalidate_tlb(ulong asid, ulong vaddr, size_t size)
{
    ulong vstart = ALIGN_DOWN(vaddr, PAGE_SIZE);
    ulong vend = ALIGN_UP(vaddr + size, PAGE_SIZE);
    ulong page_count = (vend - vstart) >> PAGE_BITS;

    ulong i;
    ulong vcur = vstart;
    for (i = 0; i < page_count; i++) {
        invalidate_tlb_line(asid, vcur);
        vcur += PAGE_SIZE;
    }
}

static void bringup_cpu(int cpu_id)
{
}

static void *init_user_page_dir()
{
//     volatile struct l1table *l1tab = (struct l1table *)PFN_TO_ADDR(page_dir_pfn);
//
//     int i;
//     for (i = 0; i < 4096; i++) {
//         l1tab->value_u32[i] = 0;
//     }
//
//     // Duplicate the last 4MB mapping
//     l1tab->value_u32[4095] = kernel_l1table->value_u32[4095];
//     l1tab->value_u32[4094] = kernel_l1table->value_u32[4094];
//     l1tab->value_u32[4093] = kernel_l1table->value_u32[4093];
//     l1tab->value_u32[4092] = kernel_l1table->value_u32[4092];
    return NULL;
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
void hal_entry(struct loader_args *largs)
{
    struct hal_arch_funcs funcs;
    memzero(&funcs, sizeof(struct hal_arch_funcs));

    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.init_int = init_int;
    funcs.init_mm = init_mm;

    funcs.putchar = raspi2_putchar;
    funcs.halt = halt_cur_cpu;

    funcs.has_direct_access = 0;
    funcs.hal_direct_access = NULL;
    funcs.map_range = map_range;
    funcs.unmap_range = unumap_range;
    funcs.translate = translate;

    funcs.get_cur_cpu_index = get_cur_cpu_index;
    funcs.bringup_cpu = bringup_cpu;

    funcs.register_drivers = register_drivers;

    funcs.get_syscall_params = get_syscall_params;
    funcs.set_syscall_return = set_syscall_return;
    funcs.handle_arch_syscall = NULL;

    funcs.arch_disable_local_int = disable_local_int;
    funcs.arch_enable_local_int = enable_local_int;

    funcs.init_addr_space = init_user_page_dir;
    funcs.init_context = init_thread_context;
    funcs.set_context_param = set_thread_context_param;
    funcs.switch_to = switch_to;
    funcs.kernel_dispatch_prep = NULL;

    funcs.invalidate_tlb = invalidate_tlb;

    hal_entry_primary(largs, &funcs);
}
