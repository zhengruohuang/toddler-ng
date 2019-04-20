#ifndef __ARCH_AARCH64_COMMON_INCLUDE_MACH_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_MACH_H__


// Raspberry PI 3
#if (defined(MACH_RASPI3))
    #define MACH_NAME "raspi3"
    #define LOADER_BASE 0x80000

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
