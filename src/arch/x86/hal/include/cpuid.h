#ifndef __ARCH_X86_HAL_INCLUDE_CPUID_H__
#define __ARCH_X86_HAL_INCLUDE_CPUID_H__


#include "common/include/inttypes.h"


enum cpuid_code {
    CPUID_CODE_VENDOR_STR = 0,
    CPUID_CODE_FEATURES,

    CPUID_CODE_TSC_FREQ = 0x15,
    CPUID_CODE_CPU_FREQ = 0x16,

    CPUID_CODE_INTEL_EXTENDED = 0x80000000,
    CPUID_CODE_INTEL_FEATURES,
    CPUID_CODE_INTEL_BRAND_STR,
    CPUID_CODE_INTEL_BRAND_STR_MORE,
    CPUID_CODE_INTEL_BRAND_STR_END,
};

enum cpuid_feature_ecx {
    CPUID_FEAT_ECX_SSE3         = 0x1 << 0,
    CPUID_FEAT_ECX_PCLMUL       = 0x1 << 1,
    CPUID_FEAT_ECX_DTES64       = 0x1 << 2,
    CPUID_FEAT_ECX_MONITOR      = 0x1 << 3,
    CPUID_FEAT_ECX_DS_CPL       = 0x1 << 4,
    CPUID_FEAT_ECX_VMX          = 0x1 << 5,
    CPUID_FEAT_ECX_SMX          = 0x1 << 6,
    CPUID_FEAT_ECX_EST          = 0x1 << 7,
    CPUID_FEAT_ECX_TM2          = 0x1 << 8,
    CPUID_FEAT_ECX_SSSE3        = 0x1 << 9,
    CPUID_FEAT_ECX_CID          = 0x1 << 10,
    CPUID_FEAT_ECX_FMA          = 0x1 << 12,
    CPUID_FEAT_ECX_CX16         = 0x1 << 13,
    CPUID_FEAT_ECX_ETPRD        = 0x1 << 14,
    CPUID_FEAT_ECX_PDCM         = 0x1 << 15,
    CPUID_FEAT_ECX_PCIDE        = 0x1 << 17,
    CPUID_FEAT_ECX_DCA          = 0x1 << 18,
    CPUID_FEAT_ECX_SSE4_1       = 0x1 << 19,
    CPUID_FEAT_ECX_SSE4_2       = 0x1 << 20,
    CPUID_FEAT_ECX_x2APIC       = 0x1 << 21,
    CPUID_FEAT_ECX_MOVBE        = 0x1 << 22,
    CPUID_FEAT_ECX_POPCNT       = 0x1 << 23,
    CPUID_FEAT_ECX_AES          = 0x1 << 25,
    CPUID_FEAT_ECX_XSAVE        = 0x1 << 26,
    CPUID_FEAT_ECX_OSXSAVE      = 0x1 << 27,
    CPUID_FEAT_ECX_AVX          = 0x1 << 28,
};

enum cpuid_feature_edx {
    CPUID_FEAT_EDX_FPU          = 0x1 << 0,
    CPUID_FEAT_EDX_VME          = 0x1 << 1,
    CPUID_FEAT_EDX_DE           = 0x1 << 2,
    CPUID_FEAT_EDX_PSE          = 0x1 << 3,
    CPUID_FEAT_EDX_TSC          = 0x1 << 4,
    CPUID_FEAT_EDX_MSR          = 0x1 << 5,
    CPUID_FEAT_EDX_PAE          = 0x1 << 6,
    CPUID_FEAT_EDX_MCE          = 0x1 << 7,
    CPUID_FEAT_EDX_CX8          = 0x1 << 8,
    CPUID_FEAT_EDX_APIC         = 0x1 << 9,
    CPUID_FEAT_EDX_SEP          = 0x1 << 11,
    CPUID_FEAT_EDX_MTRR         = 0x1 << 12,
    CPUID_FEAT_EDX_PGE          = 0x1 << 13,
    CPUID_FEAT_EDX_MCA          = 0x1 << 14,
    CPUID_FEAT_EDX_CMOV         = 0x1 << 15,
    CPUID_FEAT_EDX_PAT          = 0x1 << 16,
    CPUID_FEAT_EDX_PSE36        = 0x1 << 17,
    CPUID_FEAT_EDX_PSN          = 0x1 << 18,
    CPUID_FEAT_EDX_CLF          = 0x1 << 19,
    CPUID_FEAT_EDX_DTES         = 0x1 << 21,
    CPUID_FEAT_EDX_ACPI         = 0x1 << 22,
    CPUID_FEAT_EDX_MMX          = 0x1 << 23,
    CPUID_FEAT_EDX_FXSR         = 0x1 << 24,
    CPUID_FEAT_EDX_SSE          = 0x1 << 25,
    CPUID_FEAT_EDX_SSE2         = 0x1 << 26,
    CPUID_FEAT_EDX_SS           = 0x1 << 27,
    CPUID_FEAT_EDX_HTT          = 0x1 << 28,
    CPUID_FEAT_EDX_TM1          = 0x1 << 29,
    CPUID_FEAT_EDX_IA64         = 0x1 << 30,
    CPUID_FEAT_EDX_PBE          = 0x1 << 31,
};


#define __read_cpuid(code, a, b, c, d)              \
    __asm__ __volatile__ (                          \
        "cpuid;"                                    \
        : "=a" (a), "=b" (b), "=c" (c), "=d" (d)    \
        : "a" (code)                                \
        : "memory", "cc"                            \
    )

static inline void read_cpuid(u32 code, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx)
{
    u32 a = 0, b = 0, c = 0, d = 0;
    __read_cpuid(code, a, b, c, d);
    if (eax) *eax = a;
    if (ebx) *ebx = b;
    if (ecx) *ecx = c;
    if (edx) *edx = d;
}

static inline int read_cpuid_apic()
{
    u32 edx = 0;
    read_cpuid(CPUID_CODE_FEATURES, NULL, NULL, NULL, &edx);
    return edx & CPUID_FEAT_EDX_APIC ? 1 : 0;
}

static inline void read_cpu_mhz(u32 *base, u32 *max, u32 *bus)
{
    read_cpuid(CPUID_CODE_CPU_FREQ, base, max, bus, NULL);
}

static inline u32 read_tsc_mhz()
{
    u32 eax = 0, ebx = 0, ecx = 0;
    read_cpuid(CPUID_CODE_TSC_FREQ, &eax, &ebx, &ecx, NULL);
    u64 hz = ecx / (eax ? eax : 1);
    hz = (u64)(hz ? hz : 1) * (u64)(ebx ? ebx : 1);
    return hz >> 20;
}


#endif
