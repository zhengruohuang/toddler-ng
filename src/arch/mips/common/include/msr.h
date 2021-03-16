#ifndef __ARCH_MIPS_COMMON_INCLUDE_MSR_H__
#define __ARCH_MIPS_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


/*
 * Generic read/write
 */
#define __mfc0(value, reg, sel)             \
    __asm__ __volatile__ (                  \
        "mfc0 %[r], $" #reg ", " #sel ";"   \
        : [r] "=r" (value)                  \
        :                                   \
        : "memory"                          \
    )

#define __mtc0(value, reg, sel)             \
    __asm__ __volatile__ (                  \
        "mtc0 %[r], $" #reg ", " #sel ";"   \
        "ehb;"                              \
        :                                   \
        : [r] "r" (value)                   \
        : "memory"                          \
    )

#if (ARCH_WIDTH == 64)
#define __dmfc0(value, reg, sel)            \
    __asm__ __volatile__ (                  \
        "dmfc0 %[r], $" #reg ", " #sel ";"  \
        "ehb;"                              \
        : [r] "=r" (value)                  \
        :                                   \
        : "memory"                          \
    )

#define __dmtc0(value, reg, sel)            \
    __asm__ __volatile__ (                  \
        "dmtc0 %[r], $" #reg ", " #sel ";"  \
        "ehb;"                              \
        :                                   \
        : [r] "r" (value)                   \
        : "memory"                          \
    )
#endif


/*
 * Status
 */
struct cp0_status {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 ie      : 1;
            u32 exl     : 1;
            u32 erl     : 1;
            u32 ksu     : 2;
            u32 ux      : 1;
            u32 sx      : 1;
            u32 kx      : 1;
            u32 im0     : 1;
            u32 im1     : 1;
            u32 im2     : 1;
            u32 im3     : 1;
            u32 im4     : 1;
            u32 im5     : 1;
            u32 im6     : 1;
            u32 im7     : 1;
            u32 impl    : 2;
            u32 ase     : 1;
            u32 nmi     : 1;
            u32 sr      : 1;
            u32 ts      : 1;
            u32 bev     : 1;
            u32 px      : 1;    // Only valid in MIPS64, zero in MIPS32
            u32 mx      : 1;
            u32 re      : 1;
            u32 fr      : 1;
            u32 rp      : 1;
            u32 cu0     : 1;
            u32 cu1     : 1;
            u32 cu2     : 1;
            u32 cu3     : 1;
        };

        struct {
            u32 other0  : 8;
            u32 im      : 8;
            u32 other1  : 16;
        };
#else
        struct {
            u32 cu3     : 1;
            u32 cu2     : 1;
            u32 cu1     : 1;
            u32 cu0     : 1;
            u32 rp      : 1;
            u32 fr      : 1;
            u32 re      : 1;
            u32 mx      : 1;
            u32 px      : 1;    // Only valid in MIPS64, zero in MIPS32
            u32 bev     : 1;
            u32 ts      : 1;
            u32 sr      : 1;
            u32 nmi     : 1;
            u32 ase     : 1;
            u32 impl    : 2;
            u32 im7     : 1;
            u32 im6     : 1;
            u32 im5     : 1;
            u32 im4     : 1;
            u32 im3     : 1;
            u32 im2     : 1;
            u32 im1     : 1;
            u32 im0     : 1;
            u32 kx      : 1;
            u32 sx      : 1;
            u32 ux      : 1;
            u32 ksu     : 2;
            u32 erl     : 1;
            u32 exl     : 1;
            u32 ie      : 1;
        };

        struct {
            u32 other1  : 16;
            u32 im      : 8;
            u32 other0  : 8;
        };
#endif

        u32 value;
    };
};

#define read_cp0_status(value)   __mfc0(value, 12, 0)
#define write_cp0_status(value)  __mtc0(value, 12, 0)


/*
 * Exception
 */
struct cp0_ebase {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 cpunum      : 10;
            u32 zero        : 1;
            u32 write_gate  : 1;    // Only valid in MIPS64
            u32 base        : 20;
        };
#else
        struct {
            u32 base        : 20;
            u32 write_gate  : 1;
            u32 zero        : 1;
            u32 cpunum      : 10;
        };
#endif

        u32 value;
    };
};

#define read_cp0_ebase(value)       __mfc0(value, 15, 1)
#define write_cp0_ebase(value)      __mtc0(value, 15, 1)


/*
 * Exception 64
 */
#if (ARCH_WIDTH == 64)

struct cp0_ebase64 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u64 cpunum      : 10;
            u64 zero0       : 1;
            u64 write_gate  : 1;
            u64 base        : 52;
        };
#else
        struct {
            u64 base        : 52;
            u64 write_gate  : 1;
            u64 zero0       : 1;
            u64 cpunum      : 10;
        };
#endif

        u64 value;
    };
};

#define read_cp0_ebase64(value)     __dmfc0(value, 15, 1)
#define write_cp0_ebase64(value)    __dmtc0(value, 15, 1)

#endif


/*
 * TLB
 */
struct cp0_page_mask {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 zero0   : 11;
            u32 mask_ex : 2;
            u32 mask    : 16;
            u32 zero1   : 3;
        };
#else
        struct {
            u32 zero1   : 3;
            u32 mask    : 16;
            u32 mask_ex : 2;
            u32 zero0   : 11;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_page_grain {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 mc_cause    : 5;
            u32 zero0       : 3;
            u32 ase         : 5;
            u32 zero1       : 13;
            u32 s32         : 1;
            u32 iec         : 1;
            u32 esp         : 1;
            u32 elpa        : 1;
            u32 xie         : 1;
            u32 rie         : 1;
        };
#else
        struct {
            u32 rie         : 1;
            u32 xie         : 1;
            u32 elpa        : 1;
            u32 esp         : 1;
            u32 iec         : 1;
            u32 s32         : 1;
            u32 zero1       : 13;
            u32 ase         : 5;
            u32 zero0       : 3;
            u32 mc_cause    : 5;
        };
#endif

        u32 value;
    };
} packed4_struct;

#define read_cp0_page_mask(value)   __mfc0(value, 5, 0)
#define write_cp0_page_mask(value)  __mtc0(value, 5, 0)

#define read_cp0_page_grain(value)  __mfc0(value, 5, 1)
#define write_cp0_page_grain(value) __mtc0(value, 5, 1)

#define read_cp0_index(value)       __mfc0(value, 0, 0)
#define write_cp0_index(value)      __mtc0(value, 0, 0)

#define read_cp0_random(value)      __mfc0(value, 1, 0)


/*
 * Interrupt
 */
struct cp0_cause {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 zero0       : 2;
            u32 exc_code    : 5;
            u32 zero1       : 1;
            u32 ip0         : 1;
            u32 ip1         : 1;
            u32 ip2         : 1;
            u32 ip3         : 1;
            u32 ip4         : 1;
            u32 ip5         : 1;
            u32 ip6         : 1;
            u32 ip7         : 1;
            u32 ase         : 2;
            u32 zero2       : 3;
            u32 fdci        : 1;
            u32 wp          : 1;
            u32 iv          : 1;
            u32 ase2        : 2;
            u32 pci         : 1;
            u32 dc          : 1;
            u32 ce          : 2;
            u32 ti          : 1;
            u32 bd          : 1;
        };

        struct {
            u32 other0      : 8;
            u32 ip          : 8;
            u32 other1      : 16;
        };
#else
        struct {
            u32 bd          : 1;
            u32 ti          : 1;
            u32 ce          : 2;
            u32 dc          : 1;
            u32 pci         : 1;
            u32 ase2        : 2;
            u32 iv          : 1;
            u32 wp          : 1;
            u32 fdci        : 1;
            u32 zero2       : 3;
            u32 ase         : 2;
            u32 ip7         : 1;
            u32 ip6         : 1;
            u32 ip5         : 1;
            u32 ip4         : 1;
            u32 ip3         : 1;
            u32 ip2         : 1;
            u32 ip1         : 1;
            u32 ip0         : 1;
            u32 zero1       : 1;
            u32 exc_code    : 5;
            u32 zero0       : 2;
        };

        struct {
            u32 other1      : 16;
            u32 ip          : 8;
            u32 other0      : 8;
        };
#endif

        u32 value;
    };
} packed4_struct;

#define read_cp0_cause(value)   __mfc0(value, 13, 0)
#define write_cp0_cause(value)  __mtc0(value, 13, 0)

#if (ARCH_WIDTH == 64)
#define read_cp0_epc(value)     __dmfc0(value, 14, 0)
#define write_cp0_epc(value)    __dmtc0(value, 14, 0)
#else
#define read_cp0_epc(value)     __mfc0(value, 14, 0)
#define write_cp0_epc(value)    __mtc0(value, 14, 0)
#endif


/*
 * Processor ID
 */
struct cp0_proc_id {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 rev     : 8;
            u32 proc_id : 8;
            u32 comp_id : 8;
            u32 comp_opt: 8;
        };
#else
        struct {
            u32 comp_opt: 8;
            u32 comp_id : 8;
            u32 proc_id : 8;
            u32 rev     : 8;
        };
#endif

        u32 value;
    };
} packed4_struct;

#define read_cp0_proc_id(value)   __mfc0(value, 15, 0)


/*
 * Config
 */
struct cp0_config {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 info        : 31;
            u32 has_next    : 1;
        };
#else
        struct {
            u32 has_next    : 1;
            u32 info        : 31;
        };
#endif

        u32 value;
    };
};

struct cp0_config0 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 kseg0       : 3;
            u32 virt_icache : 1;
            u32 zero0       : 3;
            u32 mmu_type    : 3;
            u32 arch_rev    : 3;
            u32 arch_type   : 2;
            u32 big_endian  : 1;
            u32 zero1       : 9;
            u32 fixed_kuseg : 3;
            u32 fixed_k23   : 3;
            u32 has_config1 : 1;
        };
#else
        struct {
            u32 has_config1 : 1;
            u32 fixed_k23   : 3;
            u32 fixed_kuseg : 3;
            u32 zero1       : 9;
            u32 big_endian  : 1;
            u32 arch_type   : 2;
            u32 arch_rev    : 3;
            u32 mmu_type    : 3;
            u32 zero0       : 3;
            u32 virt_icache : 1;
            u32 kseg0       : 3;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_config1 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 has_fpu     : 1;
            u32 has_ejtag   : 1;
            u32 has_mips16  : 1;
            u32 has_watch   : 1;
            u32 has_perf    : 1;
            u32 has_mdmx    : 1;
            u32 has_cp2     : 1;
            u32 dcache_assoc: 3;
            u32 dcache_line : 3;
            u32 dcache_sets : 3;
            u32 icache_assoc: 3;
            u32 icache_line : 3;
            u32 icache_sets : 3;
            u32 vtlb_size   : 6;
            u32 has_config2 : 1;
        };
#else
        struct {
            u32 has_config2 : 1;
            u32 vtlb_size   : 6;
            u32 icache_sets : 3;
            u32 icache_line : 3;
            u32 icache_assoc: 3;
            u32 dcache_sets : 3;
            u32 dcache_line : 3;
            u32 dcache_assoc: 3;
            u32 has_cp2     : 1;
            u32 has_mdmx    : 1;
            u32 has_perf    : 1;
            u32 has_watch   : 1;
            u32 has_mips16  : 1;
            u32 has_ejtag   : 1;
            u32 has_fpu     : 1;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_config2 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 l2_assoc    : 4;
            u32 l2_line     : 4;
            u32 l2_sets     : 4;
            u32 l2_status   : 4;
            u32 l3_assoc    : 4;
            u32 l3_line     : 4;
            u32 l3_sets     : 4;
            u32 l3_status   : 3;
            u32 has_config3 : 1;
        };
#else
        struct {
            u32 has_config3 : 1;
            u32 l3_status   : 3;
            u32 l3_sets     : 4;
            u32 l3_line     : 4;
            u32 l3_assoc    : 4;
            u32 l2_status   : 4;
            u32 l2_sets     : 4;
            u32 l2_line     : 4;
            u32 l2_assoc    : 4;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_config3 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 reserved    : 30;
            u32 has_big_page: 1;    // Only valid in MIPS64, if set, then page mask becomes 64-bit
            u32 has_config4 : 1;
        };
#else
        struct {
            u32 has_config4 : 1;
            u32 has_big_page: 1;
            u32 reserved    : 30;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_config4 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 ftlb_sets   : 4;
            u32 ftlb_ways   : 4;
            u32 ftlb_page   : 5;
            u32 zero        : 1;
            u32 mmu_ext_type: 2;
            u32 kscr_map    : 8;
            u32 vtlb_size_ex: 4;
            u32 has_asid_ex : 1;
            u32 has_hw_inv  : 2;
            u32 has_config5 : 1;
        };

        struct {
            u32 mmu_size_ext: 8;
            u32 other       : 24;
        };
#else
        struct {
            u32 has_config5 : 1;
            u32 has_hw_inv  : 2;
            u32 has_asid_ex : 1;
            u32 vtlb_size_ex: 4;
            u32 kscr_map    : 8;
            u32 mmu_ext_type: 2;
            u32 zero        : 1;
            u32 ftlb_page   : 5;
            u32 ftlb_ways   : 4;
            u32 ftlb_sets   : 4;
        };

        struct {
            u32 other       : 24;
            u32 mmu_size_ext: 8;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_config5 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 has_nested_fault: 1;
            u32 zero0           : 1;
            u32 has_ufr         : 1;
            u32 has_mattri_regs : 1;
            u32 has_ll_bit      : 1;
            u32 has_mvcp_high   : 1;
            u32 kern_only_sdbbp : 1;
            u32 has_virt_proc   : 1;
            u32 ena_fre         : 1;
            u32 ena_ufe         : 1;
            u32 no_config2      : 1;
            u32 has_dual_endian : 1;
            u32 zero1           : 1;
            u32 no_wide_llsc    : 1;
            u32 zero2           : 13;
            u32 ena_simd        : 1;
            u32 has_eva         : 1;
            u32 dis_cache_vec   : 1;
            u32 dis_seg_ctrl    : 1;
            u32 has_config6     : 1;
        };
#else
        struct {
            u32 has_config6     : 1;
            u32 other           : 31;
        };
#endif

        u32 value;
    };
} packed4_struct;

#define read_cp0_config(idx, value)     __mfc0(value, 16, idx)
#define write_cp0_config(idx, value)    __mtc0(value, 16, idx)

#define write_cp0_wired(value)          __mtc0(value, 6, 0)


/*
 * Timer
 */
#define read_cp0_count(value)           __mfc0(value, 9, 0)
#define write_cp0_compare(value)        __mtc0(value, 11, 0)


/*
 * k0/k1
 */
#define read_k0(value)          \
    __asm__ __volatile__ (      \
        "move %[reg], $26;"     \
        : [reg] "=r" (value)    \
        :                       \
        : "memory"              \
    )
#define write_k0(value)         \
    __asm__ __volatile__ (      \
        "move $26, %[reg];"     \
        :                       \
        : [reg] "r" (value)     \
        : "memory"              \
    )

#define read_k1(value)          \
    __asm__ __volatile__ (      \
        "move %[reg], $27;"     \
        : [reg] "=r" (value)    \
        :                       \
        : "memory"              \
    )
#define write_k1(value)         \
    __asm__ __volatile__ (      \
        "move $27, %[reg];"     \
        :                       \
        : [reg] "r" (value)     \
        : "memory"              \
    )


/*
 * TLB read and probe
 */
#define do_tlb_write()                          \
    __asm__ __volatile__ (                      \
        "ehb;"      /* clear hazard barrier */  \
        "tlbwi;"    /* write indexed entry */   \
        :                                       \
        :                                       \
        : "memory"                              \
    )

#define do_tlb_probe()      \
    __asm__ __volatile__ (  \
        "ehb;"              \
        "tlbp;"             \
        :                   \
        :                   \
        : "memory"          \
    )


/*
 * TLB 64
 */
#if (ARCH_WIDTH == 64)

struct pte_context {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u64 zero    : 4;
            u64 bad_vpn2: 19;
            u64 pte_base: 41;
        };
#else
        struct {
            u64 pte_base: 41;
            u64 bad_vpn2: 19;
            u64 zero    : 4;
        };
#endif

        u64 value;
    };
} packed8_struct;

struct cp0_entry_hi {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u64 asid    : 8;
            u64 asid_ex : 2;
            u64 hw_inv  : 1;
            u64 vpn2_ex : 2;
            u64 vpn2    : 49;
            u64 region  : 2;
        };
#else
        struct {
            u64 region  : 2;
            u64 vpn2    : 49;
            u64 vpn2_ex : 2;
            u64 hw_inv  : 1;
            u64 asid_ex : 2;
            u64 asid    : 8;
        };
#endif

        u64 value;
    };
} packed8_struct;

struct cp0_entry_lo {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u64 global  : 1;
            u64 valid   : 1;
            u64 dirty   : 1;
            u64 coherent: 3;
            u64 pfn     : 49;
            u64 fill    : 7;
            u64 xi      : 1;
            u64 ri      : 1;
        };
#else
        struct {
            u64 ri      : 1;
            u64 xi      : 1;
            u64 fill    : 7;
            u64 pfn     : 49;
            u64 coherent: 3;
            u64 dirty   : 1;
            u64 valid   : 1;
            u64 global  : 1;
        };
#endif

        u64 value;
    };
} packed8_struct;

struct cp0_page_mask64 {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u64 zero0   : 11;
            u64 mask_ex : 2;
            u64 mask    : 47;
            u64 zero1   : 4;
        };
#else
        struct {
            u64 zero1   : 4;
            u64 mask    : 47;
            u64 mask_ex : 2;
            u64 zero0   : 11;
        };
#endif

        u64 value;
    };
} packed8_struct;

#define read_pte_context(value)     __dmfc0(value, 4, 0)
#define write_pte_context(value)    __dmtc0(value, 4, 0)

#define read_cp0_entry_hi(value)    __dmfc0(value, 10, 0)
#define write_cp0_entry_hi(value)   __dmtc0(value, 10, 0)

#define read_cp0_page_mask64(value)  __dmfc0(value, 5, 0)
#define write_cp0_page_mask64(value) __dmtc0(value, 5, 0)

#define read_cp0_entry_lo0(value)   __dmfc0(value, 2, 0)
#define write_cp0_entry_lo0(value)  __dmtc0(value, 2, 0)

#define read_cp0_entry_lo1(value)   __dmfc0(value, 3, 0)
#define write_cp0_entry_lo1(value)  __dmtc0(value, 3, 0)

#define read_cp0_bad_vaddr(value)   __dmfc0(value, 8, 0)


/*
 * TLB 32
 */
#else

struct pte_context {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 zero    : 4;
            u32 bad_vpn2: 19;
            u32 pte_base: 9;
        };
#else
        struct {
            u32 pte_base: 9;
            u32 bad_vpn2: 19;
            u32 zero    : 4;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_entry_hi {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 asid    : 8;
            u32 asid_ex : 2;
            u32 hw_inv  : 1;
            u32 vpn2_ex : 2;
            u32 vpn2    : 19;
        };
#else
        struct {
            u32 vpn2    : 19;
            u32 vpn2_ex : 2;
            u32 hw_inv  : 1;
            u32 asid_ex : 2;
            u32 asid    : 8;
        };
#endif

        u32 value;
    };
} packed4_struct;

struct cp0_entry_lo {
    union {
#if (ARCH_LITTLE_ENDIAN)
        struct {
            u32 global  : 1;
            u32 valid   : 1;
            u32 dirty   : 1;
            u32 coherent : 3;
            u32 pfn     : 24;
            u32 no_exec : 1;
            u32 no_read : 1;
        };
#else
        struct {
            u32 no_read : 1;
            u32 no_exec : 1;
            u32 pfn     : 24;
            u32 coherent : 3;
            u32 dirty   : 1;
            u32 valid   : 1;
            u32 global  : 1;
        };
#endif

        u32 value;
    };
} packed4_struct;

#define read_pte_context(value)     __mfc0(value, 4, 0)
#define write_pte_context(value)    __mtc0(value, 4, 0)

#define read_cp0_entry_hi(value)    __mfc0(value, 10, 0)
#define write_cp0_entry_hi(value)   __mtc0(value, 10, 0)

#define read_cp0_entry_lo0(value)   __mfc0(value, 2, 0)
#define write_cp0_entry_lo0(value)  __mtc0(value, 2, 0)

#define read_cp0_entry_lo1(value)   __mfc0(value, 3, 0)
#define write_cp0_entry_lo1(value)  __mtc0(value, 3, 0)

#define read_cp0_bad_vaddr(value)   __mfc0(value, 8, 0)

#endif


/*
 * PTE Context Reg is used for storing MP seq
 */
static inline int read_mp_seq_from_pte_context()
{
    struct pte_context c;
    read_pte_context(c.value);
    return c.pte_base;
}

static inline void write_mp_seq_to_pte_context(int mp_seq)
{
    struct pte_context c = { .value = 0 };
    c.pte_base = mp_seq;
    write_pte_context(c.value);
}


#endif
