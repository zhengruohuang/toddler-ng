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
 * Raspi3 UART
 */
#define BCM2835_BASE            (0x3f000000ul)
#define PL011_BASE              (BCM2835_BASE + 0x201000ul)
#define PL011_DR                (PL011_BASE)
#define PL011_FR                (PL011_BASE + 0x18ul)

static inline u8 _raw_mmio_read8(ulong addr)
{
    u8 val = 0;
    volatile u8 *ptr = (u8 *)addr;
    atomic_mb();
    val = *ptr;
    atomic_mb();
    return val;
}

static inline void _raw_mmio_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)addr;
    atomic_mb();
    *ptr = val;
    atomic_mb();
}

static int raspi3_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    int ready = 0;
    while (!ready) {
        ready = !(_raw_mmio_read8(PL011_FR) & 0x20);
    }

    // Write the character to the FIFO for transmission
    _raw_mmio_write8(PL011_DR, ch & 0xff);

    return 1;
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(raspi3_putchar);
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
//     init_mmu();
//     init_int_entry();
}

static void init_int_mp()
{
//     init_mmu_mp();
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
}

static void enable_local_int()
{
}

static ulong get_cur_mp_id()
{
    return 0;
}

static int get_cur_mp_seq()
{
    return 0;
}

static void register_drivers()
{
//     REGISTER_DEV_DRIVER(riscv_cpu_intc);
//     REGISTER_DEV_DRIVER(plic_intc);
//
//     REGISTER_DEV_DRIVER(clint_timer);
//
//     REGISTER_DEV_DRIVER(riscv_cpu);
}

static ulong get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    *param0 = regs->regs[1];
    *param1 = regs->regs[2];
    *param2 = regs->regs[3];

    return regs->regs[0];
}

static void set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
    regs->regs[0] = success;
    regs->regs[1] = return0;
    regs->regs[2] = return1;
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

#if (ARCH_WIDTH == 32)
    // Duplicate the last 16MB mapping
    page_table->entries[1023].value = kernel_page_table->entries[1023].value;
    page_table->entries[1022].value = kernel_page_table->entries[1022].value;
    page_table->entries[1021].value = kernel_page_table->entries[1021].value;
    page_table->entries[1020].value = kernel_page_table->entries[1020].value;
#elif (ARCH_WIDTH == 64)
    // Duplicate the last block mapping
    page_table->entries[511].value = kernel_page_table->entries[511].value;
#endif

    return page_table;
}

static void free_user_page_table(void *ptr)
{
    kernel_pfree_ptr(ptr);
}

static void init_thread_context(struct reg_context *context, ulong entry,
                                ulong param, ulong stack_top, ulong tcb, int user_mode)
{
    // Set GPRs
    memzero(context, sizeof(struct reg_context));

//     // Set param and TCB
//     context->a0 = param;
//     context->tp = tcb;
//
//     // Set PC and SP
//     context->sp = stack_top;
//     context->pc = entry;
//
//     // Status
//     struct status_reg status = { .value = 0 };
//     status.spie = 1;
//     status.spp = user_mode ? 0 : 1;
//     status.sum = 1;
//     status.mxr = 1;
//
//     context->status = status.value;
}


/*
 * AArch HAL entry
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

    funcs.putchar = raspi3_putchar;
    funcs.idle = idle_cur_cpu;
    funcs.halt = halt_cur_cpu;

    funcs.has_direct_access = 0;
    funcs.hal_map_range = generic_map_range;
    funcs.kernel_map_range = generic_map_range;
    funcs.kernel_unmap_range = generic_unmap_range;
    funcs.translate = generic_translate;

    funcs.get_cur_mp_id = get_cur_mp_id;
    funcs.get_cur_mp_seq = get_cur_mp_seq;
    funcs.mp_entry = largs->mp_entry;
    funcs.start_cpu = start_cpu;

    funcs.register_drivers = register_drivers;

    funcs.get_syscall_params = get_syscall_params;
    funcs.set_syscall_return = set_syscall_return;
    funcs.handle_arch_syscall = NULL;

    funcs.arch_disable_local_int = disable_local_int;
    funcs.arch_enable_local_int = enable_local_int;

    funcs.vaddr_limit = USER_VADDR_LIMIT;
    funcs.asid_limit = 0; // Set in init_mmu
    funcs.init_addr_space = init_user_page_table;
    funcs.free_addr_space = free_user_page_table;

    funcs.init_context = init_thread_context;
//     funcs.switch_to = switch_to;

//     funcs.kernel_pre_dispatch = kernel_pre_dispatch;
//     funcs.kernel_post_dispatch = kernel_post_dispatch;

//     funcs.invalidate_tlb = invalidate_tlb;
//     funcs.flush_tlb = flush_tlb;

    hal(largs, &funcs);
}

static void hal_entry_mp()
{
//     // Switch to CPU-private stack
//     ulong sp = get_my_cpu_init_stack_top_vaddr();
//     ulong pc = (ulong)&hal_mp;
//
//     __asm__ __volatile__ (
//         // Set up stack top
//         "mv     x2, %[sp];"
//         "addi   x2, x2, -16;"
//
//         // Jump to target
//         "mv     x6, %[pc];"
//         "jr     x6;"
//         "nop;"
//         :
//         : [pc] "r" (pc), [sp] "r" (sp)
//         : "memory"
//     );
}

void hal_entry(struct loader_args *largs, int mp)
{
    if (mp) {
//         int mp_seq = get_cur_bringup_mp_seq();
//         struct mp_context *mp_ctxt = _get_cur_mp_context();
//         mp_ctxt->seq = mp_seq;
        hal_entry_mp();
    } else {
//         struct mp_context *mp_ctxt = _get_cur_mp_context();
//         mp_ctxt->seq = 0;
        hal_entry_bsp(largs);
    }

    // Should never reach here
    panic("Should never reach here");
    while (1);
}
