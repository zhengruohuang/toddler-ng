#ifndef __ARCH_OPENRISC_COMMON_INCLUDE_MACH_H__
#define __ARCH_OPENRISC_COMMON_INCLUDE_MACH_H__


// Dummy
#if (defined(MACH_SIM))
    #define MACH_NAME "sim"

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
