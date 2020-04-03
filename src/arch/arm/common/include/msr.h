#ifndef __ARCH_ARM_COMMON_INCLUDE_MSR_H__
#define __ARCH_ARM_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * Generic write to Coprocessor
 */
#define __mcr(value, coproc, op1, op2, n, m)                                \
    __asm__ __volatile__ (                                                  \
        "mcr " #coproc ", " #op1 ", %[r], " #n ", " #m ", " #op2 ";"        \
        "isb;"                                                              \
        :                                                                   \
        : [r] "r" (value)                                                   \
        : "memory"                                                          \
    )

#define __mcr2(value, coproc, op1, op2, n, m)                               \
    __asm__ __volatile__ (                                                  \
        "mcr2 " #coproc ", " #op1 ", %[r], " n ", " m ", " #op2 ";"         \
        "isb;"                                                              \
        :                                                                   \
        : [r] "r" (value)                                                   \
        : "memory"                                                          \
    )

#define __mcrr(value1, value2, coproc, op1, m)                              \
    __asm__ __volatile__ (                                                  \
        "mcrr " #coproc ", " #op1 ", %[r1], %[r2], " #m ";"                 \
        "isb;"                                                              \
        :                                                                   \
        : [r1] "r" (value1), [r2] "r" (value2)                              \
        : "memory"                                                          \
    )

#define __mcrr2(value1, value2, coproc, op1, m)                             \
    __asm__ __volatile__ (                                                  \
        "mcrr2 " #coproc ", " #op1 ", %[r1], %[r2], " #m ";"                \
        "isb;"                                                              \
        :                                                                   \
        : [r1] "r" (value1), [r2] "r" (value2)                              \
        : "memory"                                                          \
    )



/*
 * Generic read from Coprocessor
 */
#define __mrc(value, coproc, op1, op2, n, m)                                \
    __asm__ __volatile__ (                                                  \
        "mrc " #coproc ", " #op1 ", %[r], " #n ", " #m ", " #op2 ";"        \
        : [r] "=r" (value)                                                  \
        :                                                                   \
        : "memory", "cc"                                                    \
    )

#define __mrc2(value, coproc, op1, op2, n, m)                               \
    __asm__ __volatile__ (                                                  \
        "mrc2 " #coproc ", " #op1 ", %[r], " #n ", " #m ", " #op2 ";"       \
        : [r] "=r" (value)                                                  \
        :                                                                   \
        : "memory", "cc"                                                    \
    )

#define __mrrc(value1, value2, coproc, op1, m)                              \
    __asm__ __volatile__ (                                                  \
        "mrrc " #coproc ", " #op1 ", %[r1], %[r2], " #m ";"                 \
        : [r1] "=r" (value1), [r2] "=r" (value2)                            \
        :                                                                   \
        : "memory", "cc"                                                    \
    )

#define __mrrc2(value1, value2, coproc, op1, m)                             \
    __asm__ __volatile__ (                                                  \
        "mrrc2 " #coproc ", " #op1 ", %[r1], %[r2], " #m ";"                \
        : [r1] "=r" (value1), [r2] "=r" (value2)                            \
        :                                                                   \
        : "memory", "cc"                                                    \
    )


/*
 * Generic PSR read/write
 */
#define __mrs(value, which)             \
    __asm__ __volatile__ (              \
        "mrs %[r], " #which ";"         \
        : [r] "=r" (value)              \
        :                               \
        : "memory"                      \
    )

#define __msr(value, which)             \
    __asm__ __volatile__ (              \
        "msr " #which ", %[r];"         \
        "isb;"                          \
        :                               \
        : [r] "r" (value)               \
        : "memory", "cc"                \
    )


/*
 * Address translation
 */
struct trans_tab_base_reg {
    union {
        u32 value;

        struct {
            u32 walk_inner_cacheable    : 1;
            u32 walk_inner_shared       : 1;
            u32 reserved1               : 1;
            u32 walk_outer_attributes   : 2;
            u32 reserved2               : 27;
        };

        u32 base_addr;
    };
} packed4_struct;

struct trans_tab_base_ctrl_reg {
    union {
        u32 value;

        struct {
            u32 ttbr0_width     : 3;
            u32 reserved        : 28;
            u32 ext_addr        : 1;
        };
    };
} packed4_struct;

struct domain_access_ctrl_reg {
    union {
        u32 value;

        struct {
            u32 domain0     : 2;    // 00: no access
            u32 domain1     : 2;    // 01: client, accesses are checked
            u32 domain2     : 2;    // 10: reserved, any access generates a domain fault
            u32 domain3     : 2;    // 11: manager, accesses are not checked
            u32 domain4     : 2;
            u32 domain5     : 2;
            u32 domain6     : 2;
            u32 domain7     : 2;
            u32 domain8     : 2;
            u32 domain9     : 2;
            u32 domain10    : 2;
            u32 domain11    : 2;
            u32 domain12    : 2;
            u32 domain13    : 2;
            u32 domain14    : 2;
            u32 domain15    : 2;
        };
    };
} packed4_struct;

#define read_trans_tab_base0(value)     __mrc(value, p15, 0, 0, c2, c0)
#define write_trans_tab_base0(value)    __mcr(value, p15, 0, 0, c2, c0)

#define read_trans_tab_base1(value)     __mrc(value, p15, 0, 1, c2, c0)
#define write_trans_tab_base1(value)    __mcr(value, p15, 0, 1, c2, c0)

#define read_trans_tab_base_ctrl(value) __mrc(value, p15, 0, 2, c2, c0)
#define write_trans_tab_base_ctrl(value) __mcr(value, p15, 0, 2, c2, c0)

#define read_domain_access_ctrl(value)  __mrc(value, p15, 0, 0, c3, c0)
#define write_domain_access_ctrl(value) __mcr(value, p15, 0, 0, c3, c0)


/*
 * System control
 */
struct sys_ctrl_reg {
    union {
        u32 value;

        struct {
            u32 mmu_enabled     : 1;
            u32 strict_align    : 1;
            u32 dcache_enabled  : 1;
            u32 reserved1       : 7;
            u32 swap_enabled    : 1;
            u32 bpred_enabled   : 1;
            u32 icache_enabled  : 1;
            u32 high_except_vec : 1;
            u32 rr_replacement  : 1;
            u32 reserved2       : 10;
            u32 reserved3       : 7;
        };
    };
} packed4_struct;

#define read_sys_ctrl(value)    __mrc(value, p15, 0, 0, c1, c0)
#define write_sys_ctrl(value)   __mcr(value, p15, 0, 0, c1, c0)


/*
 * Interrupt Status Register
 */
struct int_status_reg {
    union {
        u32 value;

        struct {
            u32 reserved1   : 6;
            u32 fiq         : 1;
            u32 irq         : 1;
            u32 abort       : 1;
            u32 reserved2   : 23;
        };
    };
} packed4_struct;

#define read_int_status(value)  __mrc(value, p15, 0, 0, c12, c1)
#define write_int_status(value) __mcr(value, p15, 0, 0, c12, c1)


/*
 * Software thread ID
 */
#define read_software_thread_id(value)  __mrc(value, p15, 0, 3, c13, c0)
#define write_software_thread_id(value) __mcr(value, p15, 0, 3, c13, c0)


/*
 * TLB
 */
#define inv_tlb_all()       __mcr(0, p15, 0, 0, c8, c7)
#define inv_tlb_addr(addr)  __mcr(0, p15, 0, 1, c8, c7)
#define inv_tlb_asid(asid)  __mcr(0, p15, 0, 2, c8, c7)


/*
 * Processor status
 */
struct proc_status_reg {
    union {
        u32 value;

        struct {
            u32 mode            : 5;
            u32 thumb           : 1;
            u32 fiq_mask        : 1;
            u32 irq_mask        : 1;
            u32 async_mask      : 1;
            u32 big_endian      : 1;
            u32 if_then_high    : 6;
            u32 greater_equal   : 4;
            u32 reserved        : 4;
            u32 jazelle         : 1;
            u32 if_then_low     : 2;
            u32 saturation      : 1;
            u32 overflow        : 1;
            u32 carry           : 1;
            u32 zero            : 1;
            u32 negative        : 1;
        };
    };
} packed4_struct;

#define read_current_proc_status(value)     __mrs(value, CPSR)
#define write_current_proc_status(value)    __msr(value, CPSR)

#define read_saved_proc_status(value)       __mrs(value, SPSR)
#define write_saved_proc_status(value)      __msr(value, SPSR)


/*
 * Generic timer
 */
#define read_generic_timer_freq(value)      __mrc(value, p15, 0, 0, c14, c0)

#define read_generic_timer_phys_ctrl(value)     __mrc(value, p15, 0, 1, c14, c2)
#define write_generic_timer_phys_ctrl(value)    __mcr(value, p15, 0, 1, c14, c2)

#define read_generic_timer_phys_interval(value)  __mrc(value, p15, 0, 0, c14, c2)
#define write_generic_timer_phys_interval(value) __mcr(value, p15, 0, 0, c14, c2)

#define read_generic_timer_phys_interval(value)  __mrc(value, p15, 0, 0, c14, c2)
#define write_generic_timer_phys_interval(value) __mcr(value, p15, 0, 0, c14, c2)

#define read_generic_timer_phys_compare(hi, lo)  __mrrc(lo, hi, p15, 2, c14)
#define write_generic_timer_phys_compare(hi, lo) __mcrr(lo, hi, p15, 2, c14)


/*
 * MP CPU ID
 */
struct mp_affinity_reg {
    union {
        u32 value;

        struct {
            u32 cpu_id      : 2;
            u32 reserved1   : 6;
            u32 cluster_id  : 4;
            u32 reserved2   : 20;
        };

        struct {
            u32 aff0        : 8;
            u32 aff1        : 8;
            u32 aff2        : 8;
            u32 smt         : 1;
            u32 reserved3   : 5;
            u32 uniproc     : 1;
            u32 mp_ext      : 1;
        };

        struct {
            u32 lo24        : 24;
            u32 hi8         : 8;
        };

        struct {
            u32 lo12        : 12;
            u32 hi20        : 20;
        };
    };
} packed4_struct;

#define read_cpu_id(value)     __mrc(value, p15, 0, 5, c0, c0)


#endif
