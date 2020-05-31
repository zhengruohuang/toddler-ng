#ifndef __ARCH_ALPHA_COMMON_INCLUDE_NAMES_H__
#define __ARCH_ALPHA_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_ALPHA))
    #define ARCH_NAME "alpha"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_CLIPPER))
    #define MACH_NAME "clipper"
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
