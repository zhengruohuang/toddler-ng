#ifndef __ARCH_ARM_COMMON_INCLUDE_MACH_H__
#define __ARCH_ARM_COMMON_INCLUDE_MACH_H__


// Raspberry Pi 2
#if (defined(MACH_RASPI2))
    #define MACH_NAME "raspi2"

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
