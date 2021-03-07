#include "common/include/inttypes.h"
#include "common/include/pal.h"
// #include "common/include/msr.h"
// #include "common/include/io.h"
#include "hal/include/hal.h"
// #include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/mem.h"
#include "hal/include/driver.h"


/*
 * Direct access helpers
 */
#define SEG_DIRECT_SIZE         0x20000000000ul
#define SEG_DIRECT_MAPPED       0xfffffc0000000000ul
#define SEG_DIRECT_MAPPED_IO    (SEG_DIRECT_MAPPED + 0x10000000000ul)

static inline void *cast_paddr_to_cached_seg(paddr_t paddr)
{
    panic_if(paddr >= (paddr_t)SEG_DIRECT_SIZE,
             "Invalid paddr @ %llx\n", (u64)paddr);
    ulong vaddr = cast_paddr_to_vaddr(paddr) | SEG_DIRECT_MAPPED;
    return (void *)vaddr;
}

static inline void *cast_paddr_to_io_seg(paddr_t paddr)
{
    panic_if(paddr >= (paddr_t)SEG_DIRECT_SIZE,
             "Invalid paddr @ %llx\n", (u64)paddr);
    ulong vaddr = cast_paddr_to_vaddr(paddr) | SEG_DIRECT_MAPPED_IO;
    return (void *)vaddr;
}

static inline paddr_t cast_direct_seg_to_paddr(void *ptr)
{
    ulong vaddr = (ulong)ptr;
    vaddr &= (0x1ul << (MAX_NUM_PADDR_BITS + 1)) - 0x1ul;
    return cast_vaddr_to_paddr(vaddr);
}


/*
 * Clipper UART
 */
#define SOUTH_BRIDGE_BASE_ADDR  0x1fc000000ul
#define UART_BASE_ADDR          (SOUTH_BRIDGE_BASE_ADDR + 0x3f8ul)
#define UART_DATA_ADDR          (UART_BASE_ADDR + 0x0ul)
#define UART_LINE_STAT_ADDR     (UART_BASE_ADDR + 0x5ul)

static inline u8 mmio_read8(unsigned long addr)
{
    volatile u8 *ptr = (u8 *)cast_paddr_to_io_seg(cast_vaddr_to_paddr(addr));
    u8 val = *ptr;
    return val;
}

static inline void mmio_write8(unsigned long addr, u8 val)
{
    volatile u8 *ptr = (u8 *)cast_paddr_to_io_seg(cast_vaddr_to_paddr(addr));
    *ptr = val;
}

static int clipper_putchar(int ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = mmio_read8(UART_LINE_STAT_ADDR) & 0x20;
    }

    mmio_write8(UART_DATA_ADDR, (u8)ch & 0xff);
    return 1;
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(clipper_putchar);
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
//     kprintf("enable_local_int\n");
//
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
    call_pal_wtint();
}

static void halt_cur_cpu(int count, va_list args)
{
    while (1) {
        disable_local_int();
        idle_cur_cpu();
    }
}


static void invalidate_tlb_line(ulong asid, ulong vaddr)
{
//     kprintf("TLB shootdown @ %lx, ASID: %lx ...", vaddr, asid);
//     inv_tlb_all();
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

static void start_cpu(int mp_seq, ulong mp_id, ulong entry)
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
//     context->r0 = param;
}

static void init_thread_context(struct reg_context *context, ulong entry,
                                ulong param, ulong stack_top, int user_mode)
{
//     // Set GPRs
//     memzero(context, sizeof(struct reg_context));
//
//     // Set param
//     set_thread_context_param(context, param);
//     //context->r0 = param;
//
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
 * Alpha HAL entry
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

    funcs.putchar = clipper_putchar;
    funcs.halt = halt_cur_cpu;

    //funcs.has_direct_access = 0;
    //funcs.hal_direct_access = NULL;
    //funcs.map_range = map_range;
    //funcs.unmap_range = unumap_range;
    //funcs.translate = translate;

    funcs.get_cur_mp_id = get_cur_mp_id;
    funcs.mp_entry = largs->mp_entry;
    funcs.start_cpu = start_cpu;

    funcs.register_drivers = register_drivers;

    funcs.get_syscall_params = get_syscall_params;
    funcs.set_syscall_return = set_syscall_return;
    funcs.handle_arch_syscall = NULL;

    funcs.arch_disable_local_int = disable_local_int;
    funcs.arch_enable_local_int = enable_local_int;

    funcs.init_addr_space = init_user_page_dir;
    funcs.init_context = init_thread_context;
    funcs.set_context_param = set_thread_context_param;
    //funcs.switch_to = switch_to;
    //funcs.kernel_dispatch_prep = NULL;

    funcs.invalidate_tlb = invalidate_tlb;

    hal(largs, &funcs);
}

static void hal_entry_mp()
{
//     // Switch to CPU-private stack
//     ulong sp = get_my_cpu_stack_top_vaddr();
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
