#ifndef __ARCH_M68K_COMMON_INCLUDE_MACH_H__
#define __ARCH_M68K_COMMON_INCLUDE_MACH_H__


// Dummy
#if (defined(MACH_MCF5208))
    #define MACH_NAME "mcf5208"

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
