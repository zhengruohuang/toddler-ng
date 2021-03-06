#ifndef __ARCH_RISCV_HAL_INCLUDE_VECNUM_H__
#define __ARCH_RISCV_HAL_INCLUDE_VECNUM_H__


/*
 * RISC-V exception vectors
 */
#define INT_CODE_SOFTWARE_USER          0
#define INT_CODE_SOFTWARE_SUPERVISOR    1
#define INT_CODE_SOFTWARE_HYPERVISOR    2
#define INT_CODE_SOFTWARE_MACHINE       3

#define INT_CODE_TIMER_USER             4
#define INT_CODE_TIMER_SUPERVISOR       5
#define INT_CODE_TIMER_HYPERVISOR       6
#define INT_CODE_TIMER_MACHINE          7

#define INT_CODE_EXTERNAL_USER          8
#define INT_CODE_EXTERNAL_SUPERVISOR    9
#define INT_CODE_EXTERNAL_HYPERVISOR    10
#define INT_CODE_EXTERNAL_MACHINE       11

#define EXCEPT_CODE_INSTR_MISALIGN      0
#define EXCEPT_CODE_INSTR_ACCESS        1
#define EXCEPT_CODE_INSTR_ILLEGAL       2
#define EXCEPT_CODE_BREAKPOINT          3
#define EXCEPT_CODE_LOAD_MISALIGN       4
#define EXCEPT_CODE_LOAD_ACCESS         5
#define EXCEPT_CODE_STORE_MISALIGN      6
#define EXCEPT_CODE_STORE_ACCESS        7
#define EXCEPT_CODE_ECALL_U             8
#define EXCEPT_CODE_ECALL_S             9
#define EXCEPT_CODE_ECALL_H             10
#define EXCEPT_CODE_ECALL_M             11
#define EXCEPT_CODE_INSTR_PAGE_FAULT    12
#define EXCEPT_CODE_LOAD_PAGE_FAULT     13
#define EXCEPT_CODE_STORE_PAGE_FAULT    15


#endif
