#ifndef __ARCH_SPARCV8_COMMON_INCLUDE_MACH_H__
#define __ARCH_SPARCV8_COMMON_INCLUDE_MACH_H__


// LEON3
#if (defined(MACH_LEON3))
    #define MACH_NAME "leon3"
    #define LOADER_BASE 0x40001000

// sun4m
#elif (defined(MACH_SUN4M))
    #define MACH_NAME "sun4m"
    #define LOADER_BASE 0x4000

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
