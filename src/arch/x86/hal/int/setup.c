#include "common/include/inttypes.h"
#include "common/include/compiler.h"
#include "common/include/idt.h"
#include "common/include/msr.h"
#include "hal/include/setup.h"
#include "hal/include/lib.h"
#include "hal/include/mem.h"
#include "hal/include/int.h"
#include "hal/include/mp.h"


/*
 * General int handler
 */
static void int_handler_entry(ulong except, ulong code, struct reg_context *regs)
{
    // Mark local interrupts as disabled
    disable_local_int();

    //kprintf("Int, cs: %lx, ds: %lx, gs: %lx\n", regs->cs, regs->ds, regs->gs);
    //kprintf("Int, mp seq: %d\n", arch_get_cur_mp_seq());

    //kprintf("Except: %lx, code: %lx, regs @ %p, si: %lx\n", except, code, regs, regs->si);

    // Figure out seq number
    int seq = 0;
    ulong error_code = code;

    switch (except) {
    case 0xe:
        seq = INT_SEQ_PAGE_FAULT;
        read_cr2(error_code);
        break;
    case 0x90:
        seq = INT_SEQ_SYSCALL;
        break;
    default:
        seq = INT_SEQ_PANIC;
        break;
    }

    panic_if(seq == INT_SEQ_PANIC,
             "Panic int seq! Int: %d code: %d, IP @ %lx, SP @ %lx, AX: %x\n",
             except, code, regs->ip, regs->sp, regs->ax);

    // Go to the generic handler!
    struct int_context intc;
    intc.vector = seq;
    intc.error_code = error_code;
    intc.regs = regs;

    int_handler(seq, &intc);
    //kprintf("to return!\n");

    // Restore back to previous context
    ulong kernel_cs = get_kernel_code_selector();
    int from_user = kernel_cs == regs->cs ? 0 : 1;

    set_local_int_state(1);
    restore_context_gpr(regs, from_user);

    unreachable();
}

void int_handler_prepare(struct int_reg_context *int_regs)
{
    // Replace seg regs with kernel's
    ulong kernel_cs = get_kernel_code_selector();
    if (int_regs->cs != kernel_cs) {
        load_kernel_seg();
    }

    // Save registers
    struct reg_context *regs = get_cur_int_reg_context();

    regs->ip = int_regs->ip;
    regs->bp = int_regs->bp;
    regs->flags = int_regs->flags;

    regs->cs = int_regs->cs;
    regs->ds = int_regs->ds;
    regs->es = int_regs->es;
    regs->fs = int_regs->fs;
    regs->gs = int_regs->gs;

    regs->ax = int_regs->ax;
    regs->bx = int_regs->bx;
    regs->cx = int_regs->cx;
    regs->dx = int_regs->dx;
    regs->si = int_regs->si;
    regs->di = int_regs->di;

#if (ARCH_WIDTH == 64)
    regs->r8 = int_regs->r8;
    regs->r9 = int_regs->r9;
    regs->r10 = int_regs->r10;
    regs->r11 = int_regs->r11;
    regs->r12 = int_regs->r12;
    regs->r13 = int_regs->r13;
    regs->r14 = int_regs->r14;
    regs->r15 = int_regs->r15;
#endif

    // Save SP and SS
    if (ARCH_WIDTH == 32 && int_regs->cs == kernel_cs) {
        regs->ss = int_regs->kernel_ss;
        regs->sp = (ulong)(void *)int_regs + sizeof(struct int_reg_context) - sizeof(ulong) * 2;
    } else {
        regs->ss = int_regs->ss;
        regs->sp = int_regs->sp;
    }

    //kprintf("except: %lx, code: %lx, regs @ %p, int_regs @ %p\n",
    //        int_regs->except, int_regs->code, regs, int_regs);
    //kprintf("ip: %lx, cs: %lx\n", int_regs->ip, int_regs->cs);

    // Switch stack and call real int handler
    ulong kernel_stack_top = (ulong)(void *)regs - 16;
#if (ARCH_WIDTH == 64)
    __asm__ __volatile__ (
        "movq   %%rax, %%rsp;"
        "callq  *%%rbx;"
        :
        : "D" (int_regs->except), "S" (int_regs->code), "d" ((ulong)regs),
          "a" (kernel_stack_top), "b" ((ulong)(void *)&int_handler_entry)
        : "memory"
    );

#elif (ARCH_WIDTH == 32)
    __asm__ __volatile__ (
        "movl   %%esi, %%esp;"
        "pushl  %%ecx;"
        "pushl  %%edx;"
        "pushl  %%eax;"
        "calll  *%%edi;"
        :
        : "a" (int_regs->except), "d" (int_regs->code), "c" ((ulong)regs),
          "S" (kernel_stack_top), "D" ((ulong)(void *)&int_handler_entry)
        : "memory"
    );
#endif
}


/*
 * Raw int handlers
 */
extern void raw_int_begin();
extern void raw_int_end();

static void setup_raw_int_handlers()
{
    ulong begin = (ulong)(void *)&raw_int_begin;
    ulong end = (ulong)(void *)&raw_int_end;
    ulong size = end - begin;
    panic_if(size != 32 * 256, "Unexpected raw int handler array size!\n");
}


/*
 * IDT
 */
static struct int_desc_table_ptr idtr;
static struct int_desc_table_entry idt[256];

static void setup_idt_entry(struct int_desc_table_entry *entry, ulong addr, int user)
{
    memzero(entry, sizeof(struct int_desc_table_entry));

    entry->offset_low = (u16)addr;
    entry->offset_high = (u16)(addr >> 16);
    entry->selector = get_kernel_code_selector();
    entry->type = 0xe; // IDT_GATE_INT
    entry->calling_priv = user ? 0x3 : 0;
    entry->present = 1;

#ifdef ARCH_AMD64
    entry->offset64 = addr >> 32;
#endif
}

static void setup_idt()
{
    ulong base = (ulong)(void *)&raw_int_begin;
    for (int i = 0; i < 256; i++) {
        ulong addr = base + i * 32;
        setup_idt_entry(&idt[i], addr, i == 0x90 ? 1 : 0);
    }

    idtr.base = (ulong)(void *)&idt;
    idtr.limit = sizeof(struct int_desc_table_entry) * 256;
}

static void load_idt_mp()
{
    __asm__ __volatile__ (
        "lidt (%[idtr]);"
        :
        : [idtr] "b" (&idtr)
    );
}


/*
 * Init
 */
void init_int_entry_mp()
{
    // Set up kernel stack
    struct reg_context *int_ctxt = get_cur_int_reg_context();
    ulong kernel_stack_top = (ulong)(void *)int_ctxt - 16;

    load_tss_mp(arch_get_cur_mp_seq(), kernel_stack_top);
    load_idt_mp();
}

void init_int_entry()
{
    setup_raw_int_handlers();
    setup_idt();

    init_int_entry_mp();
}
