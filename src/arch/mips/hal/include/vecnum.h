#ifndef __ARCH_MIPS_HAL_INCLUDE_VECNUM_H__
#define __ARCH_MIPS_HAL_INCLUDE_VECNUM_H__


/*
 * Raw exceptions
 */
#define EXCEPT_NUM_ILLEGAL              0
#define EXCEPT_NUM_TLB_REFILL           1
#define EXCEPT_NUM_TLB64_REFILL         2
#define EXCEPT_NUM_CACHE_ERROR          3
#define EXCEPT_NUM_GENERAL              4


/*
 * Vector numbers for internal exceptions
 */
#define MIPS_INT_SEQ_TLB_READ_ONLY      1

#define MIPS_INT_SEQ_TLB_MISS_READ      2
#define MIPS_INT_SEQ_TLB_MISS_WRITE     3

#define MIPS_INT_SEQ_ADDR_ERR_READ      4
#define MIPS_INT_SEQ_ADDR_ERR_WRITE     5

#define MIPS_INT_SEQ_BUS_ERR_READ       6
#define MIPS_INT_SEQ_BUS_ERR_WRITE      7

#define MIPS_INT_SEQ_SYSCALL            8
#define MIPS_INT_SEQ_BREAK              9
#define MIPS_INT_SEQ_RESERVED           10
#define MIPS_INT_SEQ_TRAP               13

#define MIPS_INT_SEQ_CP_UNUSABLE        11

#define MIPS_INT_SEQ_INT_OVERFLOW       12
#define MIPS_INT_SEQ_MSA_FP_EXCEPT      14
#define MIPS_INT_SEQ_FP_EXCEPT          15
#define MIPS_INT_SEQ_CP2_EXCEPT         16

#define MIPS_INT_SEQ_TLB_INHIBIT_READ   19
#define MIPS_INT_SEQ_TLB_INHIBIT_EXEC   20

#define MIPS_INT_SEQ_MSA_DISABLED       21

#define MIPS_INT_SEQ_WATCH              23
#define MIPS_INT_SEQ_MACHINE_CHECK      24
#define MIPS_INT_SEQ_THREAD             25

#define MIPS_INT_SEQ_TLB_REFILL         29
#define MIPS_INT_SEQ_CACHE_ERR          30

#define MIPS_INT_SEQ_INSTR_MDMX         22
#define MIPS_INT_SEQ_INSTR_DSP          26
#define MIPS_INT_SEQ_VIRTUL_GUEST       27


#endif
