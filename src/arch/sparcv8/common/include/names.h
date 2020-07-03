#ifndef __ARCH_SPARCV8_COMMON_INCLUDE_NAMES_H__
#define __ARCH_SPARCV8_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_SPARCV8))
    #define ARCH_NAME "sparcv8"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_LEON3))
    #define MACH_NAME "leon3"
#elif (defined(MACH_SUN4M))
    #define MACH_NAME "sun4m"
#else
    #error "Unknown machine type"
#endif

// Model names
#if (defined(MODEL_QEMU))
    #define MODEL_NAME "qemu"
#else
    #error "Unknown model type"
#endif


#endif
