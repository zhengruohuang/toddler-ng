#ifndef __ARCH_POWERPC_COMMON_INCLUDE_MACH_H__
#define __ARCH_POWERPC_COMMON_INCLUDE_MACH_H__


// Mac
#if (defined(MACH_MAC))
    #define MACH_NAME "mac"

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
