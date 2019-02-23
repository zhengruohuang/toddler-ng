#ifndef __ARCH_ARM_COMMON_INCLUDE_MACH_H__
#define __ARCH_ARM_COMMON_INCLUDE_MACH_H__


// Raspberry Pi 2
#if (defined(MACH_RASPI2))
    #define MACH_NAME "raspi2"
    #if (defined(MODEL_QEMU))
        #define LOADER_BASE 0x10000
    #else
        #define LOADER_BASE 0x8000
    #endif

// Unknown
#else
    #error "Unknown machine type"

#endif


#endif
