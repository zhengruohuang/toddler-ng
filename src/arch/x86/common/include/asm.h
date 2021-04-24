#ifndef __ARCH_X86_COMMON_INCLUDE_ASM_H__
#define __ARCH_X86_COMMON_INCLUDE_ASM_H__


#include "common/include/abi.h"


#if (ARCH_WIDTH == 64)
    #define push_ul     pushq
    #define pop_ul      popq
    #define mov_ul      movq
    #define add_ul      addq
    #define sub_ul      subq
    #define iret_ul     iretq

    #define ax_ul       rax
    #define bx_ul       rbx
    #define cx_ul       rcx
    #define dx_ul       rdx
    #define si_ul       rsi
    #define di_ul       rdi
    #define bp_ul       rbp
    #define sp_ul       rsp

    #define WORD_SIZE   8

#elif (ARCH_WIDTH == 32)
    #define push_ul     pushl
    #define pop_ul      popl
    #define mov_ul      movl
    #define add_ul      addl
    #define sub_ul      subl
    #define iret_ul     iretl

    #define ax_ul       eax
    #define bx_ul       ebx
    #define cx_ul       ecx
    #define dx_ul       edx
    #define si_ul       esi
    #define di_ul       edi
    #define bp_ul       ebp
    #define sp_ul       esp

    #define WORD_SIZE   4

#else
    #error "Unsupported arch width!"
#endif


#define __STR(s) #s
#define INST0(op)               __STR(op) ";"
#define INST1(op, reg)          __STR(op) " %%" __STR(reg) ";"
#define INST2(op, rs, rd)       __STR(op) " %%" __STR(rs)  ", %%" __STR(rd) ";"
#define INST2_IMM(op, imm, rd)  __STR(op) "  $" __STR(imm) ", %%" __STR(rd) ";"


#endif
