#ifndef __ARCH_AARCH64_COMMON_INCLUDE_NAMES_H__
#define __ARCH_AARCH64_COMMON_INCLUDE_NAMES_H__


/*
 * Arch names
 */
#if (defined(ARCH_AARCH64V8))
    #define ARCH_NAME "aarch64v8"
#else
    #error "Unknown arch type"
#endif


/*
 * Machine names
 */
#if (defined(MACH_RASPI3))
    #define MACH_NAME "raspi3"
#elif (defined(MACH_VIRT))
    #define MACH_NAME "virt"
#else
    #error "Unknown machine type"
#endif


/*
 * Model
 */
#if (defined(MODEL_QEMU))
    #define MODEL_NAME "qemu"
#elif (defined(MODEL_GENERIC))
    #define MODEL_NAME "generic"
#else
    #error "Unknown model type"
#endif


#endif
