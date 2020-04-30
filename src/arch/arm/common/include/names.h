#ifndef __ARCH_ARM_COMMON_INCLUDE_ARM_H__
#define __ARCH_ARM_COMMON_INCLUDE_ARM_H__


// Arch names
#if (defined(ARCH_ARMV7))
    #define ARCH_NAME "armv7"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_RASPI2))
    #define MACH_NAME "raspi2"
#else
    #error "Unknown machine type"
#endif

// Model names
#if (defined(MODEL_QEMU))
    #define MODEL_NAME "qemu"
#elif (defined(MODEL_GENERIC))
    #define MODEL_NAME "generic"
#else
    #error "Unknown model type"
#endif


#endif
