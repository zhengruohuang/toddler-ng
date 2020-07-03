#ifndef __ARCH_SPARCV8_COMMON_INCLUDE_MSR_H__
#define __ARCH_SPARCV8_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * ASI
 */
#ifdef MACH_LEON3
    #define ASI_CACHE_MISS  0x01
    #define ASI_CACHE_CTRL  0x02
    #define ASI_MMU_REGS    0x19
    #define ASI_MMU_BYPASS  0x1c
#else
    #define ASI_MMU_REGS    0x4
    #define ASI_MMU_BYPASS  0x20
#endif


#define asi_read32(_asi, _addr, _val)           \
    __asm__  __volatile__ (                     \
        "lda [%[addr]] %[asi], %[val];"         \
        : [val] "=r" (_val)                     \
        : [addr] "r" (_addr), [asi] "i" (_asi)  \
    )

#define asi_read8(_asi, _addr, _val)            \
    __asm__  __volatile__ (                     \
        "lduba [%[addr]] %[asi], %[val];"       \
        : [val] "=r" (_val)                     \
        : [addr] "r" (_addr), [asi] "i" (_asi)  \
    )

#define asi_write32(_asi, _addr, _val)          \
    __asm__  __volatile__ (                     \
        "sta %[val], [%[addr]] %[asi];"         \
        :                                       \
        : [val] "r" (_val),                     \
          [addr] "r" (_addr), [asi] "i" (_asi)  \
        : "memory"                              \
    )

#define asi_write8(_asi, _addr, _val)           \
    __asm__  __volatile__ (                     \
        "stuba %[val], [%[addr]] %[asi];"       \
        :                                       \
        : [val] "r" ((u8)_val),                 \
          [addr] "r" (_addr), [asi] "i" (_asi)  \
        : "memory"                              \
    )


/*
 * Generic MSR read/write
 */
#define __read_msr(value, reg)          \
    __asm__ __volatile__ (              \
        "mov %%" #reg ", %[val];"       \
        : [val] "=r" (value)            \
        :                               \
        : "memory"                      \
    )

#define __write_msr(value, reg)         \
    __asm__ __volatile__ (              \
        "mov %[val], %%" #reg ";"       \
        :                               \
        : [val] "r" (value)             \
        : "memory"                      \
    )


/*
 * Processor State Register
 */
struct proc_state {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 impl            : 4;
            u32 ver             : 4;
            u32 int_cond        : 4;
            u32 reserved        : 6;
            u32 ena_coproc      : 1;
            u32 ena_fpu         : 1;
            u32 int_level       : 4;
            u32 super           : 1;
            u32 prev_super      : 1;
            u32 ena_trap        : 1;
            u32 cur_win         : 5;
        };
#endif
    };
} packed4_struct;

#define read_proc_state(value)  __read_msr(value, psr)
#define write_proc_state(value) __write_msr(value, psr)


/*
 * Window Invalid Mask
 */
#define read_win_invalid_mask(value)    __read_msr(value, wim)
#define write_win_invalid_mask(value)   __write_msr(value, wim)


/*
 * Trap Base Register
 */
struct trap_base {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 base_addr_hi20  : 20;   // Higher 20 bits of base addr
            u32 trap_type       : 8;
            u32 zero            : 4;
        };
#endif
    };
} packed4_struct;

#define read_trap_base(value)   __read_msr(value, tbr)
#define write_trap_base(value)  __write_msr(value, tbr)


/*
 * MMU Control
 */
struct mmu_ctrl {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 impl        : 4;
            u32 ver         : 4;
            u32 sys_ctrl    : 16;
            u32 pso         : 1;    // 1: PSO or 0: TSO
            u32 reserved    : 5;
            u32 no_fault    : 1;
            u32 enabled     : 1;
        };
#endif
    };
} packed4_struct;

#define read_mmu_ctrl(value)    asi_read32(ASI_MMU_REGS, 0, value)
#define write_mmu_ctrl(value)   asi_write32(ASI_MMU_REGS, 0, value)


/*
 * MMU Context Table Pointer
 */
struct mmu_ctxt_table {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 ctxt_table_hi30 : 30;   // paddr >> 6
            u32 reserved        : 2;
        };

        struct {
            u32 ctxt_table_pfn  : 24;
            u32 zero            : 6;
            u32 reserved2       : 2;
        };
#endif
    };
} packed4_struct;

#define read_mmu_ctxt_table(value)  asi_read32(ASI_MMU_REGS, 0x100, value)
#define write_mmu_ctxt_table(value) asi_write32(ASI_MMU_REGS, 0x100, value)

#define read_mmu_ctxt_num(value)    asi_read32(ASI_MMU_REGS, 0x200, value)
#define write_mmu_ctxt_num(value)   asi_write32(ASI_MMU_REGS, 0x200, value)


/*
 * MMU Fault Status
 */
struct mmu_fault_status {
    union {
        u32 value;

#if (ARCH_LITTLE_ENDIAN)
#else
        struct {
            u32 reserved        : 14;
            u32 ext_bus_err     : 8;
            u32 level           : 2;    // 0: Ctxt table, 1/2/3: Level-1/2/3 page table
            u32 acc_type        : 3;
            u32 fault_type      : 3;
            u32 addr_valid      : 1;
            u32 overwrite       : 1;
        };
#endif
    };
} packed4_struct;

#define read_mmu_fault_status(value)    asi_read32(ASI_MMU_REGS, 0x300, value)
#define write_mmu_fault_status(value)   asi_write32(ASI_MMU_REGS, 0x300, value)

#define read_mmu_fault_addr(value)      asi_read32(ASI_MMU_REGS, 0x400, value)
#define write_mmu_fault_addr(value)     asi_write32(ASI_MMU_REGS, 0x400, value)


#endif
