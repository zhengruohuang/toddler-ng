#ifndef __ARCH_ALPHA_COMMON_INCLUDE_PAL_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_PAL_H__


#include "common/include/compiler.h"


/*
 * Common
 */
#define PAL_halt        0
#define PAL_cflush      1
#define PAL_draina      2
#define PAL_bpt         128
#define PAL_bugchk      129
#define PAL_chmk        131
#define PAL_callsys     131
#define PAL_imb         134
#define PAL_rduniq      158
#define PAL_wruniq      159
#define PAL_gentrap     170
#define PAL_nphalt	     190

/*
 * VMS
 */
#define PAL_swppal      10
#define PAL_mfpr_vptb   41

/*
 * OSF
 */
#define PAL_cserve      9
#define PAL_wripir      13
#define PAL_rdmces      16
#define PAL_wrmces      17
#define PAL_wrfen       43
#define PAL_wrvptptr    45
#define PAL_jtopal      46
#define PAL_swpctx      48
#define PAL_wrval       49
#define PAL_rdval       50
#define PAL_tbi         51
#define PAL_wrent       52
#define PAL_swpipl      53
#define PAL_rdps        54
#define PAL_wrkgp       55
#define PAL_wrusp       56
#define PAL_wrperfmon   57
#define PAL_rdusp       58
#define PAL_whami       60
#define PAL_retsys      61
#define PAL_wtint       62
#define PAL_rti         63


/*
 * Call PAL
 */
#define __call_pal(name, v0, a0, a1)                            \
    do {                                                        \
        register unsigned long r0 __asm__ ("$0");               \
        register unsigned long r16 __asm__ ("$16") = a0;        \
        register unsigned long r17 __asm__ ("$17") = a1;        \
                                                                \
        __asm__ __volatile__ (                                  \
            "call_pal %[pal_num];"                              \
            : "=r" (r16), "=r" (r17), "=r" (r0)                 \
            : [pal_num] "i" (PAL_##name), "0" (r16), "1" (r17)  \
            : "$1", "$22", "$23", "$24", "$25"                  \
        );                                                      \
                                                                \
        v0 = r0;                                                \
    } while (0)

#define __call_pal_void(name, a0, a1)                           \
    do {                                                        \
        __unused_var unsigned long ret;                         \
        __call_pal(name, ret, a0, a1);                          \
    } while (0)

static inline void call_pal_wtint()
{
    __call_pal_void(wtint, 0, 0);
}


#endif
