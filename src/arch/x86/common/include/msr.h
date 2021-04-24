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
            : "memory", "cc"                \
        )

    #define __write_cr(value, which)        \
        __asm__ __volatile__ (              \
            "movq %%rax, %%" #which ";"     \
            :                               \
            : "a" (value)                   \
            : "memory", "cc"                \
        )

#elif (defined(ARCH_IA32))
    #define __read_cr(value, which)         \
        __asm__ __volatile__ (              \
            "movl %%" #which ", %%eax;"     \
            : "=a" (value)                  \
            :                               \
            : "memory", "cc"                \
        )

    #define __write_cr(value, which)        \
        __asm__ __volatile__ (              \
            "movl %%eax, %%" #which ";"     \
            :                               \
            : "a" (value)                   \
            : "memory", "cc"                \
        )

#endif

#define __read_cr32(value, which)       \
    __asm__ __volatile__ (              \
        "movl %%" #which ", %%eax;"     \
        : "=a" (value)                  \
        :                               \
        : "memory", "cc"                \
    )

#define __write_cr32(value, which)      \
    __asm__ __volatile__ (              \
        "movl %%eax, %%" #which ";"     \
        :                               \
        : "a" (value)                   \
        : "memory", "cc"                \
    )


/*
 * Generic MSR read and write
 */
#if (defined(ARCH_AMD64))
    #define __read_msr(value, addr)             \
        __asm__ __volatile__ (                  \
            "rdmsr;"                            \
            "shl $32, %%rdx;"                   \
            "or  %%rdx, %%rax;"                 \
            : "=a" (value)                      \
            : "c" (addr)                        \
            : "%rdx", "memory", "cc"            \
        )

    #define __write_msr(value, addr)                \
        __asm__ __volatile__ (                      \
            "wrmsr;"                                \
            :                                       \
            : "c" (addr),                           \
              "a" ((ulong)(value) & 0xfffffffful),  \
              "d" ((ulong)(value) >> 32)            \
            : "memory", "cc"                        \
        )

#elif (defined(ARCH_IA32))
    #define __read_msr(value, addr)             \
        __asm__ __volatile__ (                  \
            "rdmsr;"                            \
            : "=a" (value)                      \
            : "c" (addr)                        \
            : "memory", "cc"                    \
        )

    #define __write_msr(value, addr)            \
        __asm__ __volatile__ (                  \
            "wrmsr;"                            \
            :                                   \
            : "c" (addr), "a" (value), "d" (0)  \
            : "memory", "cc"                    \
        )

#endif

#define __read_msr32(value, addr)           \
    __asm__ __volatile__ (                  \
        "rdmsr;"                            \
        : "=a" (value)                      \
        : "c" (addr)                        \
        : "memory", "cc"                    \
    )

#define __write_msr32(value, addr)          \
    __asm__ __volatile__ (                  \
        "wrmsr;"                            \
        :                                   \
        : "c" (addr), "a" (value), "d" (0)  \
        : "memory", "cc"                    \
    )


/*
 * Read segment registers
 */
#define __read_seg_reg(value, which)    \
    __asm__ __volatile__ (              \
        "xorl %%eax, %%eax;"            \
        "movw %%" #which ", %%ax;"      \
        :                               \
        : "a" (value)                   \
        : "memory", "cc"                \
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
 * CR2 - page fault vaddr
 */
#define read_cr2(value)     __read_cr(value, cr2)
#define read_cr2_32(value)  __read_cr32(value, cr2)


/*
 * CR3 - page table base
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
 * Segment registers
 */
#define read_cs(value)      __read_seg_reg(value, cs)
#define read_ds(value)      __read_seg_reg(value, ds)
#define read_es(value)      __read_seg_reg(value, es)
#define read_fs(value)      __read_seg_reg(value, fs)
#define read_gs(value)      __read_seg_reg(value, gs)
#define read_ss(value)      __read_seg_reg(value, ss)

#ifdef ARCH_AMD64

#define read_fsbase(value)  __read_msr(value, 0xc0000100)
#define write_fsbase(value) __write_msr(value, 0xc0000100)

#define read_gsbase(value)  __read_msr(value, 0xc0000101)
#define write_gsbase(value) __write_msr(value, 0xc0000101)

#define read_gsbase_kernel(value)   __read_msr(value, 0xc0000102)
#define write_gsbase_kernel(value)  __write_msr(value, 0xc0000102)

#endif


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

#define read_efer(value)    __read_msr32(value, 0xc0000080)
#define write_efer(value)   __write_msr32(value, 0xc0000080)


/*
 * APIC_BASE
 */
struct apic_base_reg {
    union {
        ulong value;

        struct {
            ulong reserved0         : 8;
            ulong is_bootstrap      : 1;
            ulong reserved1         : 1;
            ulong x2apic_enabled    : 1;
            ulong xapic_enabled     : 1;
            ulong apic_base_pfn     : sizeof(ulong) * 8 - 12;
        };
    };
} natural_struct;

#define read_apic_base(value)   __read_msr(value, 0x1b)
#define write_apic_base(value)  __write_msr(value, 0x1b)


#endif
