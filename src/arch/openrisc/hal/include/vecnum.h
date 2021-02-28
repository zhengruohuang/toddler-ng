#ifndef __ARCH_OPENRISC_HAL_INCLUDE_VECNUM_H__
#define __ARCH_OPENRISC_HAL_INCLUDE_VECNUM_H__

/*
 * Raw exceptions
 */
#define EXCEPT_NUM_UNKNOWN              0
#define EXCEPT_NUM_RESET                1
#define EXCEPT_NUM_BUS_ERROR            2
#define EXCEPT_NUM_DATA_PAGE_FAULT      3
#define EXCEPT_NUM_INSTR_PAGE_FAULT     4
#define EXCEPT_NUM_TIMER                5
#define EXCEPT_NUM_ALIGN                6
#define EXCEPT_NUM_ILLEGAL_INSTR        7
#define EXCEPT_NUM_INTERRUPT            8
#define EXCEPT_NUM_DTLB_MISS            9
#define EXCEPT_NUM_ITLB_MISS            10
#define EXCEPT_NUM_ARITH_OVERFLOW       11
#define EXCEPT_NUM_SYSCALL              12
#define EXCEPT_NUM_FLOAT                13
#define EXCEPT_NUM_TRAP                 14
#define EXCEPT_NUM_RESERVED             15

#endif
