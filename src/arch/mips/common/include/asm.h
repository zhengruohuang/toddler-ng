#ifndef __ARCH_MIPS_COMMON_INCLUDE_ASM_H__
#define __ARCH_MIPS_COMMON_INCLUDE_ASM_H__


#include "common/include/abi.h"


/*
 * Symbolic register names
 */
#define zero    $0      // wired zero
#define AT      $1      // assembler temp  - uppercase because of ".set at"
#define v0      $2      // return value
#define v1      $3
#define a0      $4      // argument registers
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8      // caller saved
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16     // callee saved
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24     // caller saved
#define t9      $25
#define jp      $25     // PIC jump register
#define k0      $26     // kernel scratch
#define k1      $27
#define gp      $28     // global pointer
#define sp      $29     // stack pointer
#define fp      $30     // frame pointer
#define s8      $30     // same like fp!
#define ra      $31     // return address


/*
 * Symbolic register names for CP0
 */
#define CP0_INDEX       $0
#define CP0_RANDOM      $1
#define CP0_ENTRYLO0    $2
#define CP0_ENTRYLO1    $3
#define CP0_CONTEXT     $4
#define CP0_PAGEMASK    $5
#define CP0_WIRED       $6
#define CP0_BADVADDR    $8
#define CP0_COUNT       $9
#define CP0_ENTRYHI     $10
#define CP0_COMPARE     $11
#define CP0_STATUS      $12
#define CP0_CAUSE       $13
#define CP0_EPC         $14
#define CP0_PRID        $15
#define CP0_CONFIG      $16
#define CP0_LLADDR      $17
#define CP0_WATCHLO     $18
#define CP0_WATCHHI     $19
#define CP0_XCONTEXT    $20
#define CP0_FRAMEMASK   $21
#define CP0_DIAGNOSTIC  $22
#define CP0_PERFORMANCE $25
#define CP0_ECC         $26
#define CP0_CACHEERR    $27
#define CP0_TAGLO       $28
#define CP0_TAGHI       $29
#define CP0_ERROREPC    $30


/*
 * Width-generic instructions
 */
#if (ARCH_WIDTH == 32)
#define li_ul       li
#define la_ul       la
#define ld_ul       lw
#define ll_ul       ll
#define st_ul       sw
#define sc_ul       sc
#define addi_ul     addi
#define addu_ul     addu
#define sub_ul      sub
#define sll_ul      sll

#elif (ARCH_WIDTH == 64)
#define li_ul       dli
#define la_ul       dla
#define ld_ul       ld
#define ll_ul       lld
#define st_ul       sd
#define sc_ul       scd
#define addi_ul     daddi
#define addu_ul     daddu
#define sub_ul      dsub
#define sll_ul      dsll

#else
#error "Unsupported arch width"
#endif


#endif
