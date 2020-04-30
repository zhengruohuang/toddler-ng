#ifndef __ARCH_MIPS_COMMON_INCLUDE_NAMES_H__
#define __ARCH_MIPS_COMMON_INCLUDE_NAMES_H__


// Arch names
#if (defined(ARCH_MIPS32B))
    #define ARCH_NAME "mips32b"
#elif (defined(ARCH_MIPS32L))
    #define ARCH_NAME "mips32l"
#elif (defined(ARCH_MIPS64B))
    #define ARCH_NAME "mips64b"
#elif (defined(ARCH_MIPS64L))
    #define ARCH_NAME "mips64l"
#else
    #error "Unknown arch type"
#endif

// Machine names
#if (defined(MACH_MALTA))
    #define MACH_NAME "malta"
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
