#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/page.h"
#include "common/include/asm.h"
#include "common/include/msr.h"
#include "hal/include/hal.h"
#include "hal/include/setup.h"
#include "hal/include/kprintf.h"
#include "hal/include/lib.h"
#include "hal/include/mp.h"
#include "hal/include/mem.h"
#include "hal/include/driver.h"
#include "hal/include/kernel.h"


/*
 * x86 PC UART
 */
#define SERIAL_PORT (0x3f8)
#define SERIAL_DR   (SERIAL_PORT)
#define SERIAL_FR   (SERIAL_PORT + 0x5)

static inline u8 ioport_read8(u16 port)
{
    u8 data;

    __asm__ __volatile__ (
        "inb %[port], %[data];"
        : [data] "=a" (data)
        : [port] "Nd" (port)
        : "memory"
    );

    return data;
}

static inline void ioport_write8(u16 port, u8 data)
{
    __asm__ __volatile__ (
        "outb %[data], %[port];"
        :
        : [data] "a" (data), [port] "Nd" (port)
        : "memory"
    );
}

static int x86_pc_putchar(int ch)
{
    // Wait until the UART has an empty space in the FIFO
    u32 empty = 0;
    while (empty) {
        empty = !(ioport_read8(SERIAL_FR) & 0x20);
    }

    // Write the character to the FIFO for transmission
    ioport_write8(SERIAL_DR, ch & 0xff);

    return 1;
}


/*
 * Per-arch funcs
 */
static void init_libk()
{
    init_libk_putchar(x86_pc_putchar);
    init_libk_page_table(pre_palloc, NULL, NULL);
}

static void init_arch()
{
}

static void init_arch_mp()
{
}

static void init_per_cpu_pre()
{
    init_segment();
    load_gdt_mp(0);
    setup_mp_seq();

    init_mp_boot();
}

static void init_int()
{
    init_mmu();
    init_int_entry();
}

static void init_int_mp()
{
    init_mmu_mp();
    init_int_entry_mp();
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
 * IO port
 */
static ulong ioport_read(ulong port, int size)
{
    u16 port16 = port;
    ulong val = 0;

    if (size == 1) {
        u8 data = 0;
        __asm__ __volatile__ (
            "inb %[port], %[data];"
            : [data] "=a" (data)
            : [port] "Nd" (port16)
            : "memory"
        );
        val = data;
    } else if (size == 2) {
        u16 data = 0;
        __asm__ __volatile__ (
            "inw %[port], %[data];"
            : [data] "=a" (data)
            : [port] "d" (port16)
            : "memory"
        );
        val = data;
    } else if (size == 4) {
        u32 data = 0;
        __asm__ __volatile__ (
            "inl %[port], %[data];"
            : [data] "=a" (data)
            : [port] "d" (port16)
            : "memory"
        );
        val = data;
    } else {
        panic("Unknown data size: %d\n", size);
    }

    return val;
}

void ioport_write(ulong port, int size, ulong val)
{
    u16 port16 = port;

    if (size == 1) {
        u8 data = val;
        __asm__ __volatile__ (
            "outb %[data], %[port];"
            :
            : [data] "a" (data), [port] "d" (port16)
            : "memory"
        );
    } else if (size == 2) {
        u16 data = val;
        __asm__ __volatile__ (
            "outw %[data], %[port];"
            :
            : [data] "a" (data), [port] "d" (port16)
            : "memory"
        );
    } else if (size == 4) {
        u32 data = val;
        __asm__ __volatile__ (
            "outl %[data], %[port];"
            :
            : [data] "a" (data), [port] "d" (port16)
            : "memory"
        );
    } else {
        panic("Unknown data size: %d\n", size);
    }
}


/*
 * Simple and quick funcs
 */
static void disable_local_int()
{
    __asm__ __volatile__ ( "cli;" :: );
}

static void enable_local_int()
{
     __asm__ __volatile__ ( "sti;" :: );
}

static void register_drivers()
{
    REGISTER_DEV_DRIVER(local_apic);
    REGISTER_DEV_DRIVER(io_apic);

    REGISTER_DEV_DRIVER(x86_cpu);
}

static ulong get_syscall_params(struct reg_context *regs, ulong *param0, ulong *param1, ulong *param2)
{
    *param0 = regs->ax;
    *param1 = regs->bx;
    *param2 = regs->cx;

    return regs->si;
}

static void set_syscall_return(struct reg_context *regs, int success, ulong return0, ulong return1)
{
    regs->si = success;
    regs->ax = return0;
    regs->bx = return1;
}

static void idle_cur_cpu()
{
    __asm__ __volatile__ ( "hlt;" : : : "memory" );
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
//     sbi_hart_start(mp_id, entry, 0);
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

    // Set param and TCB
    context->ax = param;
    context->di = param;
//     context->tp = tcb;

    // Set PC and SP
    context->sp = stack_top - sizeof(ulong) * 2;
    context->ip = entry;

    // Seg regs
    context->cs = get_segment_selector('c', user_mode);
    context->ds = get_segment_selector('d', user_mode);
    context->es = get_segment_selector('e', user_mode);
    context->fs = get_segment_selector('f', user_mode);
    context->gs = get_segment_selector('g', user_mode);
    context->ss = get_segment_selector('s', user_mode);

    // Flags
    context->flags = 0x200202;
}


/*
 * x86 HAL entry
 */
static void hal_entry_bsp(struct loader_args *largs)
{
    struct hal_arch_funcs funcs;
    memzero(&funcs, sizeof(struct hal_arch_funcs));

    funcs.init_libk = init_libk;
    funcs.init_arch = init_arch;
    funcs.init_arch_mp = init_arch_mp;
    funcs.init_per_cpu_pre = init_per_cpu_pre;
    funcs.init_int = init_int;
    funcs.init_int_mp = init_int_mp;
    funcs.init_mm = init_mm;
    funcs.init_mm_mp = init_mm_mp;
    funcs.init_kernel_pre = init_kernel_pre;
    funcs.init_kernel_post = init_kernel_post;

    funcs.has_ioport = 1;
    funcs.ioport_read = ioport_read;
    funcs.ioport_write = ioport_write;

    funcs.putchar = x86_pc_putchar;
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
    funcs.switch_to = switch_to;

    funcs.kernel_pre_dispatch = kernel_pre_dispatch;
    funcs.kernel_post_dispatch = kernel_post_dispatch;

    funcs.invalidate_tlb = invalidate_tlb;
    funcs.flush_tlb = flush_tlb;

    hal(largs, &funcs);
}

static void hal_entry_mp()
{
    // Switch to CPU-private stack
    ulong sp = get_my_cpu_init_stack_top_vaddr();
    ulong pc = (ulong)&hal_mp;

    __asm__ __volatile__ (
        INST2(mov_ul, ax_ul, sp_ul)
        INST2_IMM(sub_ul, 16, sp_ul)
        INST1(call, bx_ul)
        :
        : "b" (pc), "a" (sp)
        : "memory"
    );

    unreachable();
}

void hal_entry(struct loader_args *largs, int mp)
{
    if (mp) {
        int mp_seq = get_cur_bringup_mp_seq();
        kprintf("CPU #%d starting\n", mp_seq);
        load_gdt_mp(mp_seq);
        kprintf("MP seq: %d\n", arch_get_cur_mp_seq());
        hal_entry_mp();
    } else {
        hal_entry_bsp(largs);
    }

    unreachable();
}
