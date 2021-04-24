#ifndef __ARCH_X86_COMMON_INCLUDE_TSS_H__
#define __ARCH_X86_COMMON_INCLUDE_TSS_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


#if (ARCH_WIDTH == 32)
struct task_state_segment {
    u32 prev;
    u32 sp0;        // stack pointer to use during interrupt
    u32 ss0;        // stack segment to use during interrupt
    u32 sp1;
    u32 ss1;
    u32 sp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 flags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldtr;
    u16 trap;
    u16 iopb_addr;
    // Note that if the base address of IOPB is greater than or equal to the
    // limit(size) of the TSS segment (normally it would be 104),
    // it means that there's no IOPB
} packed4_struct;

#elif (ARCH_WIDTH == 64)
struct task_state_segment {
    u32 reserved;
    u64 sp0;
    u64 sp1;
    u64 sp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 trap;
    u16 iopb_addr;
} packed4_struct;

#else
#error "Unsupported arch width!"

#endif


#endif
