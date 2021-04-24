#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "common/include/abi.h"
#include "common/include/asm.h"
#include "hal/include/kprintf.h"
#include "hal/include/setup.h"


void restore_context_gpr(struct reg_context *regs, int user_mode)
{
    struct int_reg_context *int_regs = ARCH_WIDTH == 64 || user_mode ?
        (void *)((ulong)(void *)regs - sizeof(struct int_reg_context) - 16) :
        (void *)(regs->sp - sizeof(struct int_reg_context) + sizeof(ulong) * 2);

    //kprintf("Restore, regs @ %p, int_regs %p\n", regs, int_regs);
    //kprintf("CS: %lx, IP: %lx, SS: %lx, DS: %lx, SP: %lx\n",
    //        regs->cs, regs->ip, regs->ss, regs->ds, regs->sp);

    // Restore es, fs, and gs
    load_program_seg(regs->es, regs->fs, regs->gs);

    // Prepare registers
    int_regs->ip = regs->ip;
    int_regs->bp = regs->bp;
    int_regs->flags = regs->flags;

    int_regs->cs = regs->cs;
    int_regs->ds = regs->ds;
    int_regs->es = regs->es;
    int_regs->fs = regs->fs;
    int_regs->gs = regs->gs;

    int_regs->ax = regs->ax;
    int_regs->bx = regs->bx;
    int_regs->cx = regs->cx;
    int_regs->dx = regs->dx;
    int_regs->si = regs->si;
    int_regs->di = regs->di;

#if (ARCH_WIDTH == 64)
    int_regs->r8 = regs->r8;
    int_regs->r9 = regs->r9;
    int_regs->r10 = regs->r10;
    int_regs->r11 = regs->r11;
    int_regs->r12 = regs->r12;
    int_regs->r13 = regs->r13;
    int_regs->r14 = regs->r14;
    int_regs->r15 = regs->r15;
#endif

    if (ARCH_WIDTH == 64 || user_mode) {
        int_regs->ss = regs->ss;
        int_regs->sp = regs->sp;
    }

    // Restore registers
    __asm__ __volatile__ (
        // Set up stack
        INST2(mov_ul, ax_ul, sp_ul)

        // Seg regs
        INST2_IMM(add_ul, WORD_SIZE, sp_ul) // Skip ss
        INST2_IMM(add_ul, WORD_SIZE, sp_ul) // Skip gs
        INST2_IMM(add_ul, WORD_SIZE, sp_ul) // Skip fs
        INST2_IMM(add_ul, WORD_SIZE, sp_ul) // Skip es
        INST1(pop_ul, ax_ul)
        "movw   %%ax, %%ds;"                // Restore ds

#if (ARCH_WIDTH == 64)
        // AMD64 additional GPRs
        "popq   %%r8;"
        "popq   %%r9;"
        "popq   %%r10;"
        "popq   %%r11;"
        "popq   %%r12;"
        "popq   %%r13;"
        "popq   %%r14;"
        "popq   %%r15;"
#endif

        // GPRs
        INST1(pop_ul, di_ul)
        INST1(pop_ul, si_ul)
        INST1(pop_ul, bp_ul)
        INST1(pop_ul, bx_ul)
        INST1(pop_ul, dx_ul)
        INST1(pop_ul, cx_ul)
        INST1(pop_ul, ax_ul)

        // Skip except and code
        INST2_IMM(add_ul, WORD_SIZE, sp_ul)
        INST2_IMM(add_ul, WORD_SIZE, sp_ul)

        // Return
        INST0(iret_ul)

// #if (ARCH_WIDTH == 64)
//         "movq   %%rax, %%rsp;"
//
//         // Skip ss
//         "add    $8, %%rsp;"
//
//         // Restore seg regs
//         "popq   %%rax;"
//         //"movw   %%ax, %%gs;"
//         "popq   %%rax;"
//         //"movw   %%ax, %%fs;"
//         "popq   %%rax;"
//         "movw   %%ax, %%es;"
//         "popq   %%rax;"
//         "movw   %%ax, %%ds;"
//
//         // GPRs
//         "popq   %%r8;"
//         "popq   %%r9;"
//         "popq   %%r10;"
//         "popq   %%r11;"
//         "popq   %%r12;"
//         "popq   %%r13;"
//         "popq   %%r14;"
//         "popq   %%r15;"
//
//         "popq   %%rdi;"
//         "popq   %%rsi;"
//         "popq   %%rbp;"
//         "addq   $8, %%rsp;"
//
//         "popq   %%rbx;"
//         "popq   %%rdx;"
//         "popq   %%rcx;"
//         "popq   %%rax;"
//
//         // Skip except and code
//         "addq   $16, %%rsp;"
//
//         // Return
//         "iretq;"
//
// #elif (ARCH_WIDTH == 32)
//         "movl   %%eax, %%esp;"
//
//         // Skip ss
//         "addl   $4, %%esp;"
//
//         // Restore seg regs
//         "popl   %%eax;"
//         "movw   %%ax, %%gs;"
//         "popl   %%eax;"
//         "movw   %%ax, %%fs;"
//         "popl   %%eax;"
//         "movw   %%ax, %%es;"
//         "popl   %%eax;"
//         "movw   %%ax, %%ds;"
//
//         // GPRs
//         "popal;"
//
//         // Skip except and code
//         "addl   $8, %%esp;"
//
//         // Return
//         "iretl;"
// #endif
        :
        : "a" ((ulong)(void *)int_regs)
        : "memory", "cc"
    );
}
