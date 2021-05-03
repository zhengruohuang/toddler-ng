#ifndef __ARCH_AARCH64_COMMON_INCLUDE_MSR_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * Generic MSR read and write
 */
#define __mrs(value, systemreg)             \
    __asm__ __volatile__ (                  \
        "mrs %[r], " #systemreg ";"         \
        : [r] "=r" (value)                  \
        :                                   \
        : "memory"                          \
    )

#define __msr(value, systemreg)             \
    __asm__ __volatile__ (                  \
        "msr " #systemreg ", %[r];"         \
        "isb;"                              \
        :                                   \
        : [r] "r" (value)                   \
        : "memory"                          \
    )


/*
 * SCTLR
 * System control register
 */
struct sys_ctrl_reg {
    union {
        u32 value;

        struct {
            u32 mmu             : 1;    // MMU enable for EL1 and EL0 stage 1 address translation
            u32 align_check     : 1;    // Alignment check enable
            u32 data_cachable   : 1;    // Cacheability control, for data accesses
            u32 sa              : 1;    // SP Alignment check enable
            u32 sa0             : 1;    // SP Alignment check enable for EL0
            u32 cp15ben         : 1;    // System instruction memory barrier enable
            u32 reserved9       : 1;    // res0
            u32 itd             : 1;    // IT Disable
            u32 sed             : 1;    // SETEND instruction disable
            u32 uma             : 1;    // User Mask Access
            u32 reserved8       : 1;    // res0
            u32 reserved7       : 1;    // res1
            u32 instr_cacheable : 1;    // Instruction access Cacheability control
            u32 endb            : 1;    // res0
            u32 dze             : 1;    // Traps EL0 execution of DC ZVA instructions to EL1
            u32 uct             : 1;    // Traps EL0 accesses to the CTR_EL0 to EL1
            u32 ntwi            : 1;    // Traps EL0 execution of WFI instructions to EL1
            u32 reserved6       : 1;    // res0
            u32 ntwe            : 1;    // Traps EL0 execution of WFE instructions to EL1
            u32 wxn             : 1;    // Write permission implies XN (Execute-never)
            u32 reserved5       : 1;    // res1
            u32 iesb            : 1;    // res0
            u32 reserved4       : 1;    // res1
            u32 span            : 1;    // res1
            u32 big_endian_e0   : 1;    // Endianness of data accesses at EL0
            u32 big_endian      : 1;    // Endianness of data accesses at EL1, and stage 1 translation table walks in the EL1&0 translation regime
            u32 uci             : 1;    // Traps EL0 execution of cache maintenance instructions to EL1
            u32 enda            : 1;    // res0
            u32 ntlsmd          : 1;    // res1
            u32 lsmaoe          : 1;    // res1
            u32 enib            : 1;    // res0
            u32 enia            : 1;    // res0
        };
    };
} packed_struct;

#define read_sys_ctrl_el1(value)    __mrs(value, sctlr_el1)
#define write_sys_ctrl_el1(value)   __msr(value, sctlr_el1)


/*
 * MAIR
 * Memory attribute indirection register
 */
#define MAIR_OUTER_DEV              0
#define MAIR_OUTER_NON_CACHEABLE    0x4
#define MAIR_OUTER_NORMAL           0xf

#define MAIR_INNER_DEV_NGNRNE       0x0
#define MAIR_INNER_DEV_NGNRE        0x4
#define MAIR_INNER_DEV_NGRE         0x8
#define MAIR_INNER_DEV_GRE          0xc

#define MAIR_INNER_NON_CACHEABLE    0x4
#define MAIR_INNER_NORMAL           0xf

#define MAIR_IDX_NORMAL             0
#define MAIR_IDX_DEVICE             1
#define MAIR_IDX_NON_CACHEABLE      2

struct mair_attri {
    union {
        u8 value;
        struct {
            u8  inner   : 4;
            u8  outer   : 4;
        };
    };
} packed_struct;

struct mem_attri_indirect_reg {
    union {
        u64 value;
        struct mair_attri attri[8];
    };
} packed_struct;

#define read_mem_attri_indirect_el1(value)  __mrs(value, mair_el1)
#define write_mem_attri_indirect_el1(value) __msr(value, mair_el1)


/*
 * TCR
 * Translation control register
 *
 * Intermediate Physical Address Size
 *  000 32 bits, 4GB
 *  001 36 bits, 64GB
 *  010 40 bits, 1TB
 *  011 42 bits, 4TB
 *  100 44 bits, 16TB
 *  101 48 bits, 256TB
 *  110 52 bits, 4PB
 *  Other values are reserved
 *
 * Granule size for TTBR1
 *  01 16KB
 *  10 4KB
 *  11 64KB
 *
 * Granule size for the TTBR0
 *  00 4KB
 *  01 64KB
 *  10 16KB
 *
 * Shareability
 *  00 Non-shareable
 *  10 Outer Shareable
 *  11 Inner Shareable
 *
 * Outer cacheability
 *  00 Normal memory, Outer Non-cacheable
 *  01 Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
 *  10 Normal memory, Outer Write-Through Read-Allocate No Write-Allocate Cacheable
 *  11 Normal memory, Outer Write-Back Read-Allocate No Write-Allocate Cacheable
 *
 * Inner cacheability
 *  00 Normal memory, Inner Non-cacheable
 *  01 Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
 *  10 Normal memory, Inner Write-Through Read-Allocate No Write-Allocate Cacheable
 *  11 Normal memory, Inner Write-Back Read-Allocate No Write-Allocate Cacheable
 *
 * Size offset (SZ)
 *  The size offset of the memory region addressed by TTBR?_EL1
 *  The region size is 2^(64-SZ) bytes
 */
struct trans_ctrl_reg {
    union {
        u64 value;

        struct {
            u64 size_offset0        : 6;    // t0sz
            u64 reserved3           : 1;    // res0
            u64 walk_disable0       : 1;    // epd0
            u64 inner_cacheable0    : 2;    // igrn0
            u64 outer_cacheable0    : 2;    // orgn0
            u64 shareability0       : 2;    // sh0
            u64 ttbr_granule0       : 2;    // tg0
            u64 size_offset1        : 6;    // t1sz
            u64 asid_in_which_ttbr  : 1;    // a0
            u64 walk_disable1       : 1;    // epd1
            u64 inner_cacheable1    : 2;    // irgn1
            u64 outer_cacheable1    : 2;    // orgn1
            u64 shareability1       : 2;    // sh1
            u64 ttbr_granule1       : 2;    // tg1
            u64 interm_paddr_size   : 3;    // ips
            u64 reserved2           : 1;    // res0
            u64 asid_16bit          : 1;    // as
            u64 top_byte_ignored0   : 1;    // tbi0
            u64 top_byte_ignored1   : 1;    // tbi1
            u64 reserved1           : 25;   // res0
        };
    };
} packed_struct;

#define read_trans_ctrl_el1(value)  __mrs(value, tcr_el1)
#define write_trans_ctrl_el1(value) __msr(value, tcr_el1)


/*
 * TTBR
 * Translation table base register
 */
struct trans_tab_base_reg {
    union {
        u64 value;

        struct {
            u64 cnp         : 1;
            u64 base_addr   : 47;
            u64 asid        : 16;
        };
    };
} packed_struct;

#define read_trans_tab_base0_el1(value)     __mrs(value, ttbr0_el1)
#define write_trans_tab_base0_el1(value)    __msr(value, ttbr0_el1)

#define read_trans_tab_base1_el1(value)     __mrs(value, ttbr1_el1)
#define write_trans_tab_base1_el1(value)    __msr(value, ttbr1_el1)


/*
 * ID_AA64MMFR0
 * AArch64 Memory Model Feature Register 0
 */
#define ID_AA64MMFR0_GRANULE_SUPPORT        0x0
#define ID_AA64MMFR0_GRANULE_NOT_SUPPORT    0xf

struct aarch64_mem_model_feature_reg {
    union {
        u64 value;

        struct {
            u64 paddr_range : 4;    // 0000=32bit, 0001=36bits, 0010=40bits, 0011=42bits, 0100=44bits, 0101=48bits, 0110=52bits
            u64 asid_bits   : 4;    // 0010=16bit, 0000=8bits
            u64 big_end     : 4;    // Mixed-endian support
            u64 scure       : 4;    // 0001=Support a distinction between scure and non-scure memory
            u64 big_end_el0 : 4;    // 0001=Mixed-endian support at EL0
            u64 granule_64k : 4;
            u64 granule_4k  : 4;    // 0000=supported, 1111=not supported
            u64 reserved1   : 32;   // res0
        };
    };
} packed_struct;

#define read_aarch64_mem_model_feature_el1(value)   __mrs(value, id_aa64mmfr0_el1)
#define write_aarch64_mem_model_feature_el1(value)  __msr(value, id_aa64mmfr0_el1)


/*
 * SCR
 * Secure Configuration Register
 */
struct secure_config_reg {
    union {
        u32 value;

        struct {
            u32 ns          : 1;    // EL0&1 are in Non-secure state. Memory accesses from EL0&1 cannot access Secure memory
            u32 irq         : 1;    // Physical IRQ interrupts are taken to EL3
            u32 fiq         : 1;    // Physical FIQ interrupts are taken to EL3
            u32 ea          : 1;    // External aborts and SError interrupts are taken to EL3
            u32 reserved2   : 2;    // res1
            u32 reserved1   : 1;    // res0
            u32 smd         : 1;    // Disable secure monitor call
            u32 hce         : 1;    // Enable hypervisor call instruction
            u32 sif         : 1;    // Disable secure instr fetch from non-secure memory
            u32 rw          : 1;    // Execution state control for lower Exception levels
            u32 st          : 1;    // Traps Secure EL1 accesses to the Counter-timer Physical Secure timer registers to EL3, from AArch64 state only
            u32 twi         : 1;    // Trap TWI to EL3
            u32 twe         : 1;    // Trap WFE to EL3
            u32 tlor        : 1;    // res0
            u32 terr        : 1;    // Trap error record accesses, res0
            u32 apk         : 1;    // res0
            u32 api         : 1;    // res0
            u32 reserved0   : 14;   // res0
        };
    };
} packed_struct;

#define read_secure_config_el3(value)   __mrs(value, scr_el3)
#define write_secure_config_el3(value)  __msr(value, scr_el3)


/*
 * HCR
 * Hypervisor Configuration Register
 */
struct hyp_config_reg {
    union {
        u64 value;

        struct {
            u64 vm          : 1;    // Virtualization enable
            u64 swio        : 1;    // Set/Way Invalidation Override, res1
            u64 ptw         : 1;    // Protected Table Walk
            u64 fmo         : 1;    // Physical FIQ Routing
            u64 imo         : 1;    // Physical IRQ Routing
            u64 amo         : 1;    // Physical SError interrupt routing
            u64 vf          : 1;    // Virtual FIQ Interrupt
            u64 vi          : 1;    // Virtual IRQ Interrupt
            u64 vse         : 1;    // Virtual SError interrupt
            u64 fb          : 1;    // Force broadcast
            u64 bsu         : 2;    // Barrier Shareability upgrade
            u64 dc          : 1;    // Default Cacheability
            u64 twi         : 1;    // Traps Non-secure EL0 and EL1 execution of WFI instructions to EL2, from both Execution states
            u64 twe         : 1;    // Traps Non-secure EL0 and EL1 execution of WFE instructions to EL2, from both Execution states
            u64 tid0        : 1;    // Trap ID group 0
            u64 tid1        : 1;    // Trap ID group 1
            u64 tid2        : 1;    // Trap ID group 2
            u64 tid3        : 1;    // Trap ID group 3
            u64 tsc         : 1;    // Trap SMC instructions
            u64 tidcp       : 1;    // Trap IMPLEMENTATION DEFINED functionality
            u64 tacr        : 1;    // Trap Auxiliary Control Registers
            u64 tsw         : 1;    // Trap data or unified cache maintenance instructions that operate by Set/Way
            u64 tpcp        : 1;    // Trap data or unified cache maintenance instructions that operate to the Point of Coherency or Persistence
            u64 tpu         : 1;    // Trap cache maintenance instructions that operate to the Point of Unification
            u64 ttlb        : 1;    // Trap TLB maintenance instructions
            u64 tvm         : 1;    // Trap Virtual Memory controls
            u64 tge         : 1;    // Trap General Exceptions
            u64 tdz         : 1;    // Trap DC ZVA instructions
            u64 hcd         : 1;    // HVC instruction disable
            u64 trvm        : 1;    // Trap Reads of Virtual Memory controls
            u64 rw          : 1;    // Execution state control for lower Exception levels, 0 = AArch32, 1 = AArch64
            u64 cd          : 1;    // Stage 2 Data access cacheability disable
            u64 id          : 1;    // Stage 2 Instruction access cacheability disable
            u64 e2h         : 1;    // res0
            u64 tlor        : 1;    // res0
            u64 terr        : 1;    // Trap Error record accesses
            u64 tea         : 1;    // Route synchronous External abort exceptions to EL2
            u64 miocnce     : 1;    // Mismatched Inner/Outer Cacheable Non-Coherency Enable, for non-secure EL0 & 1
            u64 reserved1   : 1;    // res0
            u64 apk         : 1;    // res0
            u64 api         : 1;    // res0
            u64 nv          : 1;    // res0
            u64 nv1         : 1;    // res0
            u64 at          : 1;    // res0
            u64 reserved2   : 19;   // res0

        };
    };
} packed_struct;

#define read_hyp_config_el2(value)   __mrs(value, hcr_el2)
#define write_hyp_config_el2(value)  __msr(value, hcr_el2)


/*
 * CNTHCTL
 * Counter-timer Hypervisor Control Register
 */
struct cnt_timer_hyp_ctrl_reg {
    union {
        u32 value;
        struct {
            u32 el1pcten    : 1;    // Traps Non-secure EL0 and EL1 accesses to the physical counter register to EL2
            u32 el1pcen     : 1;    // Traps Non-secure EL0 and EL1 accesses to the physical timer registers to EL2
            u32 evnten      : 1;    // Enables the generation of an event stream from the counter register CNTPCT_EL0
            u32 evntdir     : 1;    // Controls which transition of the counter register CNTPCT_EL0 trigger bit, 0 = 0->1, 1 = 1->0
            u32 evnti       : 4;    // Selects which bit (0 to 15) of the counter register CNTPCT_EL0 is the trigger
            u32 reserved    : 24;   // res0
        };
    };
} packed_struct;

#define read_cnt_timer_hyp_ctrl_el2(value)  __mrs(value, cnthctl_el2)
#define write_cnt_timer_hyp_ctrl_el2(value) __msr(value, cnthctl_el2)

#define read_cnt_timer_virt_offset_el2(value)   __mrs(value, cntvoff_el2)
#define write_cnt_timer_virt_offset_el2(value)  __msr(value, cntvoff_el2)


/*
 * Current EL
 */
struct cur_except_level_reg {
    union {
        u32 value;

        struct {
            u32 reserved0   : 2;
            u32 except_level: 2;
            u32 reserved1   : 28;
        };
    };
} packed_struct;

#define read_cur_except_level(value)    __mrs(value, CurrentEL)
#define write_cur_except_level(value)   __msr(value, CurrentEL)


/*
 * ELR
 */
struct except_link_reg {
    union {
        u64 value;
        u64 ret_addr;
    };
} packed_struct;

#define read_except_link_el1(value)     __mrs(value, elr_el1)
#define write_except_link_el1(value)    __msr(value, elr_el1)

#define read_except_link_el2(value)     __mrs(value, elr_el2)
#define write_except_link_el2(value)    __msr(value, elr_el2)

#define read_except_link_el3(value)     __mrs(value, elr_el3)
#define write_except_link_el3(value)    __msr(value, elr_el3)


/*
 * SP and SPSel
 * Stack Pointer and Stack Pointer Select
 */
struct stack_ptr_reg {
    union {
        u64 value;
        u64 sp;
    };
} packed_struct;

#define read_stack_ptr_el0(value)   __mrs(value, sp_el0)
#define write_stack_ptr_el0(value)  __msr(value, sp_el0)

#define read_stack_ptr_el1(value)   __mrs(value, sp_el1)
#define write_stack_ptr_el1(value)  __msr(value, sp_el1)

#define read_stack_ptr_el2(value)   __mrs(value, sp_el2)
#define write_stack_ptr_el2(value)  __msr(value, sp_el2)

#define read_stack_ptr_el3(value)   __mrs(value, sp_el3)
#define write_stack_ptr_el3(value)  __msr(value, sp_el3)

struct stack_ptr_sel_reg {
    union {
        u32 value;

        union {
            u32 indiv_sp    : 1;    // 1 = Use individual SP for ELx
            u32 reserved    : 31;
        };
    };
} packed_struct;

#define read_stack_ptr_sel(value)   __mrs(value, spsel)
#define write_stack_ptr_sel(value)  __msr(value, spsel)


/*
 * SPSR
 * Saved Program Status Register
 */
struct saved_program_status_reg {
    union {
        u32 value;

        struct {
            u32 indiv_sp    : 1;    // SP sel
            u32 reserved4   : 1;    // res0
            u32 from_el     : 2;    // EL
            u32 from_a32    : 1;    // 0 = From AArch64
            u32 reserved3   : 1;    // res0
            u32 f           : 1;    // FIQ exception maksed
            u32 i           : 1;    // IRQ exception maksed
            u32 a           : 1;    // SError exception masked
            u32 d           : 1;    // Watchpoint, Breakpoint, and Software Step exceptions targeted at the current EL are masked
            u32 reserved2   : 10;   // res0
            u32 il          : 1;    // Illegal execution, PSTATE.IL
            u32 ss          : 1;    // Software step, PSTATE.SS
            u32 pan         : 1;    // res0
            u32 uao         : 1;    // res0
            u32 reserved1   : 4;    // res0
            u32 v           : 1;    // CPSR.V
            u32 c           : 1;    // CPSR.C
            u32 z           : 1;    // CPSR.Z
            u32 n           : 1;    // CPSR.N
        };
    };
} packed_struct;

#define read_saved_program_status_abt(value)    __mrs(value, spsr_abt)
#define write_saved_program_status_abt(value)   __msr(value, spsr_abt)

#define read_saved_program_status_fiq(value)    __mrs(value, spsr_fiq)
#define write_saved_program_status_fiq(value)   __msr(value, spsr_fiq)

#define read_saved_program_status_irq(value)    __mrs(value, spsr_irq)
#define write_saved_program_status_irq(value)   __msr(value, spsr_irq)

#define read_saved_program_status_und(value)    __mrs(value, spsr_und)
#define write_saved_program_status_und(value)   __msr(value, spsr_und)

#define read_saved_program_status_el1(value)    __mrs(value, spsr_el1)
#define write_saved_program_status_el1(value)   __msr(value, spsr_el1)

#define read_saved_program_status_el2(value)    __mrs(value, spsr_el2)
#define write_saved_program_status_el2(value)   __msr(value, spsr_el2)

#define read_saved_program_status_el3(value)    __mrs(value, spsr_el3)
#define write_saved_program_status_el3(value)   __msr(value, spsr_el3)


#endif
