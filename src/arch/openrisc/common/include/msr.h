#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_MSR_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


/*
 * Generic read/write
 */
#define __mfspr(value, group, reg)                              \
    __asm__ __volatile__ (                                      \
        "l.mfspr %[val], %[spr], 0;"                            \
        : [val] "=r" (value)                                    \
        : [spr] "r" (((group) << 11) | (reg))                   \
        : "memory"                                              \
    )

#define __mtspr(value, group, reg)                              \
    __asm__ __volatile__ (                                      \
        "l.mtspr %[spr], %[val], 0;"                            \
        :                                                       \
        : [val] "r" (value), [spr] "r" (((group) << 11) | (reg))\
        : "memory"                                              \
    )


/*
 * Unit Present Reg
 */
struct unit_present_reg {
    union {
        u32 value;

        struct {
            u32 custom          : 8;
            u32 reserved        : 13;
            u32 tick_timer      : 1;
            u32 intc            : 1;
            u32 power_mgmt      : 1;
            u32 perf_count      : 1;
            u32 debug_unit      : 1;
            u32 mac             : 1;
            u32 immu            : 1;
            u32 dmmu            : 1;
            u32 icache          : 1;
            u32 dcache          : 1;
            u32 valid           : 1;
        };
    };
} packed4_struct;

#define read_upr(value)         __mfspr(value, 0, 1)
#define write_upr(value)        __mtspr(value, 0, 1)


/*
 * CPU Config Reg
 */
struct cpu_config_reg {
    union {
        u32 value;

        struct {
            u32 reserved                : 17;
            u32 has_arith_except_regs   : 1;
            u32 has_impl_regs           : 1;
            u32 has_except_base_reg     : 1;
            u32 has_arch_ver_reg        : 1;
            u32 no_delay_slot           : 1;
            u32 has_orvdx64             : 1;
            u32 has_orfpx64             : 1;
            u32 has_orfpx32             : 1;
            u32 has_orbis64             : 1;
            u32 has_orbis32             : 1;
            u32 has_custom_gpr          : 1;
            u32 num_shadow_gpr_files    : 4;
        };
    };
} packed4_struct;

#define read_cpucfgr(value)     __mfspr(value, 0, 2)
#define write_cpucfgr(value)    __mtspr(value, 0, 2)


/*
 * Supervision
 */
struct supervision_reg {
    union {
        u32 value;

        struct {
            u32 fast_cid            : 4;
            u32 reserved            : 11;
            u32 spr_allowed_in_user : 1;
            u32 reserved_one        : 1;
            u32 high_except_vectors : 1;
            u32 delay_slot_except   : 1;
            u32 overflow_as_except  : 1;
            u32 overflow_flag       : 1;
            u32 carry_flag          : 1;
            u32 cond_br_flag        : 1;
            u32 cid_enabled         : 1;
            u32 little_endian       : 1;
            u32 immu_enabled        : 1;
            u32 dmmu_enabled        : 1;
            u32 icache_enabled      : 1;
            u32 dcache_enabled      : 1;
            u32 int_enabled         : 1;
            u32 timer_enabled       : 1;
            u32 supervisor          : 1;
        };
    };
} packed4_struct;

#define read_sr(value)          __mfspr(value, 0, 17)
#define write_sr(value)         __mtspr(value, 0, 17)


/*
 * Context Reg
 */
struct cur_context_reg {
    union {
        u32 value;

        struct {
            u32 ccid            : 16;
            u32 ccrs            : 16;
        };
    };
} packed4_struct;


/*
 * MMU Config Reg
 */
struct mmu_config_reg {
    union {
        u32 value;

        struct {
            u32 reserved            : 20;
            u32 has_hardware_walker : 1;
            u32 has_invalidate_reg  : 1;
            u32 has_protect_reg     : 1;
            u32 has_ctrl_reg        : 1;
            u32 num_atb_entrnies    : 3;
            u32 num_tlb_sets_order  : 3;
            u32 num_tlb_ways        : 2;
        };
    };
} packed4_struct;

#define read_immucfgr(value)    __mfspr(value, 0, 4)
#define write_immucfgr(value)   __mtspr(value, 0, 4)

#define read_dmmucfgr(value)    __mfspr(value, 0, 3)
#define write_dmmucfgr(value)   __mtspr(value, 0, 3)

#define read_mmucfgr(value, i)  __mfspr(value, 0, i ? 4 : 3)
#define write_mmucfgr(value, i) __mtspr(value, 0, i ? 4 : 3)


/*
 * MMU Ctrl Reg
 */
struct mmu_ctrl_reg {
    union {
        u32 value;

        struct {
            u32 page_table_hi22 : 22;
            u32 reserved        : 9;
            u32 flush           : 1;
        };
    };
} packed4_struct;

#define read_immucr(value)      __mfspr(value, 2, 0)
#define write_immucr(value)     __mtspr(value, 2, 0)

#define read_dmmucr(value)      __mfspr(value, 1, 0)
#define write_dmmucr(value)     __mtspr(value, 1, 0)

#define read_mmucr(value, i)    __mfspr(value, i ? 2 : 1, 0)
#define write_mmucr(value, i)   __mtspr(value, i ? 2 : 1, 0)


/*
 * MMU Protect Reg
 */
struct immu_protect_reg {
    union {
        u32 value;

        struct {
            u32 reserved : 18;
            u32 group7  : 2;
            u32 group6  : 2;
            u32 group5  : 2;
            u32 group4  : 2;
            u32 group3  : 2;
            u32 group2  : 2;
            u32 group1  : 2;
        };
    };
} packed4_struct;

struct dmmu_protect_reg {
    union {
        u32 value;

        struct {
            u32 reserved : 4;
            u32 group7  : 4;
            u32 group6  : 4;
            u32 group5  : 4;
            u32 group4  : 4;
            u32 group3  : 4;
            u32 group2  : 4;
            u32 group1  : 4;
        };
    };
} packed4_struct;

#define read_immupr(value)      __mfspr(value, 2, 1)
#define write_immupr(value)     __mtspr(value, 2, 1)

#define read_dmmupr(value)      __mfspr(value, 1, 1)
#define write_dmmupr(value)     __mtspr(value, 1, 1)

#define read_mmupr(value, i)    __mfspr(value, i ? 2 : 1, 1)
#define write_mmupr(value, i)   __mtspr(value, i ? 2 : 1, 1)


/*
 * TLB Match Reg
 */
struct tlb_match_reg {
    union {
        u32 value;

        struct {
            u32 vpn         : 19;
            u32 reserved    : 5;
            u32 lru_age     : 2;
            u32 cid         : 4;
            u32 page_level1 : 1;
            u32 valid       : 1;
        };
    };
} packed4_struct;

#define MAX_TLB_LRU_AGE 3

#define read_dtlbmr(value, way, set)    __mfspr(value, 1, 512 + (way) * 256 + (set))
#define write_dtlbmr(value, way, set)   __mtspr(value, 1, 512 + (way) * 256 + (set))

#define read_itlbmr(value, way, set)    __mfspr(value, 2, 512 + (way) * 256 + (set))
#define write_itlbmr(value, way, set)   __mtspr(value, 2, 512 + (way) * 256 + (set))

#define read_tlbmr(value, i, way, set)  __mfspr(value, i ? 2 : 1, 512 + (way) * 256 + (set))
#define write_tlbmr(value, i, way, set) __mtspr(value, i ? 2 : 1, 512 + (way) * 256 + (set))


/*
 * TLB Translation Reg
 */
struct dtlb_trans_reg {
    union {
        u32 value;

        struct {
            u32 ppn             : 19;
            u32 reserved        : 3;
            u32 kernel_write    : 1;
            u32 kernel_read     : 1;
            u32 user_write      : 1;
            u32 user_read       : 1;
            u32 dirty           : 1;
            u32 accessed        : 1;
            u32 weak_order      : 1;
            u32 write_back      : 1;
            u32 cache_disabled  : 1;
            u32 coherent        : 1;
        };
    };
} packed4_struct;

struct itlb_trans_reg {
    union {
        u32 value;

        struct {
            u32 ppn             : 19;
            u32 reserved        : 5;
            u32 user_exec       : 1;
            u32 kernel_exec     : 1;
            u32 dirty           : 1;
            u32 accessed        : 1;
            u32 weak_order      : 1;
            u32 write_back      : 1;
            u32 cache_disabled  : 1;
            u32 coherent        : 1;
        };
    };
} packed4_struct;

#define read_dtlbtr(value, way, set)    __mfspr(value, 1, 640 + (way) * 256 + (set))
#define write_dtlbtr(value, way, set)   __mtspr(value, 1, 640 + (way) * 256 + (set))

#define read_itlbtr(value, way, set)    __mfspr(value, 2, 640 + (way) * 256 + (set))
#define write_itlbtr(value, way, set)   __mtspr(value, 2, 640 + (way) * 256 + (set))

#define read_tlbtr(value, i, way, set)  __mfspr(value, i ? 2 : 1, 640 + (way) * 256 + (set))
#define write_tlbtr(value, i, way, set) __mtspr(value, i ? 2 : 1, 640 + (way) * 256 + (set))


/*
 * Exception
 */
struct except_base_reg {
    union {
        u32 value;

        struct {
            u32 ppn             : 19;
            u32 reserved        : 13;
        };
    };
} packed4_struct;

#define read_evbar(value)       __mfspr(value, 0, 11)
#define write_evbar(value)      __mtspr(value, 0, 11)

#define read_eear(value, idx)   __mfspr(value, 0, 48 + idx)
#define read_eear0(value)       __mfspr(value, 0, 48)

#define read_epcr(value, idx)   __mfspr(value, 0, 32 + idx)
#define read_epcr0(value)       __mfspr(value, 0, 32)
#define write_epcr0(value)      __mtspr(value, 0, 32)

#define read_esr(value, idx)    __mfspr(value, 0, 64 + idx)
#define read_esr0(value)        __mfspr(value, 0, 64)
#define write_esr0(value)       __mtspr(value, 0, 64)


/*
 * Shadow registers
 */
#define read_gpr0(value, idx)   __mfspr(value, 0, 1024 + idx)
#define write_gpr0(value, idx)  __mtspr(value, 0, 1024 + idx)

#define read_gpr1(value, idx)   __mfspr(value, 0, 1024 + 32 + idx)
#define write_gpr1(value, idx)  __mtspr(value, 0, 1024 + 32 + idx)

#define read_gpr2(value, idx)   __mfspr(value, 0, 1024 + 64 + idx)
#define write_gpr2(value, idx)  __mtspr(value, 0, 1024 + 64 + idx)


/*
 * PIC
 */
#define read_pic_mask(value)    __mfspr(value, 9, 0)
#define write_pic_mask(value)   __mtspr(value, 9, 0)

#define read_pic_status(value)  __mfspr(value, 9, 2)
#define write_pic_status(value) __mtspr(value, 9, 2)


/*
 * Timer
 */
struct tick_mode_reg {
    union {
        u32 value;

        struct {
            u32 mode        : 2;
            u32 int_enabled : 1;
            u32 int_pending : 1;
            u32 period      : 28;
        };
    };
} packed4_struct;

#define read_tick_mode(value)   __mfspr(value, 10, 0)
#define write_tick_mode(value)  __mtspr(value, 10, 0)

#define read_tick_count(value)  __mfspr(value, 10, 1)
#define write_tick_count(value) __mtspr(value, 10, 1)


/*
 * Power mgmt
 */
struct power_mgmt_reg {
    union {
        u32 value;

        struct {
            u32 reserved        : 24;
            u32 suspend         : 1;
            u32 dyn_clk_gate    : 1;
            u32 sleep           : 1;
            u32 doze            : 1;
            u32 log_clk_reduce  : 4;
        };
    };
} packed4_struct;

#define read_pmr(value)         __mfspr(value, 8, 0)
#define write_pmr(value)        __mtspr(value, 8, 0)


/*
 * MP
 */
#define read_core_id(value)     __mfspr(value, 0, 128)
#define read_num_cores(value)   __mfspr(value, 0, 129)


#endif
