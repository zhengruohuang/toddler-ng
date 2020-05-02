#ifndef __ARCH_SH4_COMMON_INCLUDE_NAMES_H__
#define __ARCH_SH4_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_SH4))
    #define ARCH_NAME "sh4"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_R2D))
    #define MACH_NAME "r2d"
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
