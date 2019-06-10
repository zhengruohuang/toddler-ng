#ifndef __ARCH_RISCV_COMMON_INCLUDE_MACH_H__
#define __ARCH_RISCV_COMMON_INCLUDE_MACH_H__


// SiFive U
#if (defined(MACH_VIRT))
    #define MACH_NAME "virt"

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
