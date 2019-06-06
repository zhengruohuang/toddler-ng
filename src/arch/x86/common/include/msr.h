#ifndef __ARCH_X86_COMMON_INCLUDE_MSR_H__
#define __ARCH_X86_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * Generic CR read and write
 */
#if (defined(ARCH_AMD64))

#define __read_cr(value, which)         \
    __asm__ __volatile__ (              \
        "movq %%" #which ", %%rax;"     \
        : "=a" (value)                  \
        :                               \
    )

#define __write_cr(value, which)        \
    __asm__ __volatile__ (              \
        "movq %%rax, %%" #which ";"     \
        :                               \
        : "a" (value)                   \
    )

#elif (defined(ARCH_IA32))

#define __read_cr(value, which)         \
    __asm__ __volatile__ (              \
        "movl %%" #which ", %%eax;"     \
        : "=a" (value)                  \
        :                               \
    )

#define __write_cr(value, which)        \
    __asm__ __volatile__ (              \
        "movl %%eax, %%" #which ";"     \
        :                               \
        : "a" (value)                   \
    )

#endif

#define __read_cr32(value, which)       \
    __asm__ __volatile__ (              \
        "movl %%" #which ", %%eax;"     \
        : "=a" (value)                  \
        :                               \
    )

#define __write_cr32(value, which)      \
    __asm__ __volatile__ (              \
        "movl %%eax, %%" #which ";"     \
        :                               \
        : "a" (value)                   \
    )


/*
 * Generic MSR read and write
 */
#define __read_msr(value, addr)         \
    __asm__ __volatile__ (              \
        "rdmsr;"                        \
        : "=a" (value)                  \
        : "c" (addr)                    \
    )

#define __write_msr(value, addr)        \
    __asm__ __volatile__ (              \
        "wrmsr;"                        \
        :                               \
        : "a" (value), "c" (addr)       \
    )


/*
 * CR0
 */
struct ctrl_reg0 {
    union {
        u32 value;

        struct {
            u32 protect_mode    : 1;
            u32 monitor_coproc  : 1;
            u32 emulation       : 1;
            u32 task_switched   : 1;
            u32 ext_type        : 1;
            u32 numeric_error   : 1;
            u32 reserved0       : 10;
            u32 write_protect   : 1;
            u32 reserved1       : 1;
            u32 align_mask      : 1;
            u32 reserved2       : 10;
            u32 no_write_thru   : 1;
            u32 cache_disabled  : 1;
            u32 paging          : 1;
        };
    };
} packed_struct;

#define read_cr0(value)     __read_cr(value, cr0)
#define write_cr0(value)    __write_cr(value, cr0)

#define read_cr0_32(value)  __read_cr32(value, cr0)
#define write_cr0_32(value) __write_cr32(value, cr0)


/*
 * CR3
 */
struct ctrl_reg3 {
    union {
        u32 value;

        struct {
            u32 pcid        : 12;
            u32 pfn         : 20;
        };
    };
} packed_struct;

#define read_cr3(value)     __read_cr(value, cr3)
#define write_cr3(value)    __write_cr(value, cr3)

#define read_cr3_32(value)  __read_cr32(value, cr3)
#define write_cr3_32(value) __write_cr32(value, cr3)


/*
 * CR4
 */
struct ctrl_reg4 {
    union {
        u32 value;

        struct {
            u32 v8086           : 1;
            u32 pmode_virt_int  : 1;
            u32 timestamp_disab : 1;
            u32 debug_ext       : 1;
            u32 page_size_ext   : 1;
            u32 phys_addr_ext   : 1;
            u32 machine_check   : 1;
            u32 page_global     : 1;
            u32 perf_monitor    : 1;
            u32 os_fxsr         : 1;
            u32 os_xmm_except   : 1;
            u32 umode_instr_prev: 1;
            u32 reserved0       : 1;
            u32 vm_ext_enab     : 1;
            u32 safer_mode_enab : 1;
            u32 reserved1       : 2;
            u32 pcid_enabled    : 1;
            u32 os_xsave        : 1;
            u32 reserved2       : 1;
            u32 kernel_exec_prot: 1;
            u32 kernel_acc_prot : 1;
            u32 protect_key     : 1;
            u32 reserved3       : 9;
        };
    };
} packed_struct;

#define read_cr4(value)     __read_cr(value, cr4)
#define write_cr4(value)    __write_cr(value, cr4)

#define read_cr4_32(value)  __read_cr32(value, cr4)
#define write_cr4_32(value) __write_cr32(value, cr4)


/*
 * EFER
 */
struct ext_feature_enable_reg {
    union {
        u32 value;

        struct {
            u32 syscall_ext     : 1;
            u32 reserved0       : 7;
            u32 long_mode       : 1;
            u32 reserved1       : 1;
            u32 long_mode_act   : 1;
            u32 no_exec         : 1;
            u32 secure_vm       : 1;
            u32 lmode_seg_limit : 1;
            u32 fast_fxsr       : 1;
            u32 trans_cache_ext : 1;
            u32 reserved2       : 16;
        };
    };
} packed_struct;

#define read_efer(value)    __read_msr(value, 0xc0000080)
#define write_efer(value)   __write_msr(value, 0xc0000080)


#endif
