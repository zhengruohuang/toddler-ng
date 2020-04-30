#ifndef __ARCH_M68K_COMMON_INCLUDE_NAMES_H__
#define __ARCH_M68K_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_M68K))
    #define ARCH_NAME "m68k"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_MCF5208))
    #define MACH_NAME "mcf5208"
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
