#ifndef __ARCH_RISCV_COMMON_INCLUDE_NAMES_H__
#define __ARCH_RISCV_COMMON_INCLUDE_NAMES_H__


/*
 * Arch names
 */
#if (defined(ARCH_RISCV32))
    #define ARCH_NAME "riscv32"
#elif (defined(ARCH_RISCV64))
    #define ARCH_NAME "riscv64"
#else
    #error "Unknown arch type"
#endif


/*
 * Machine names
 */
#if (defined(MACH_SPIKE))
    #define MACH_NAME "spike"
#elif (defined(MACH_SIFIVE_U))
    #define MACH_NAME "sifive_u"
#elif (defined(MACH_VIRT))
    #define MACH_NAME "virt"
#else
    #error "Unknown machine type"
#endif


/*
 * Model
 */
#if (defined(MODEL_GENERIC))
    #define MODEL_NAME "generic"
#elif (defined(MODEL_QEMU))
    #define MODEL_NAME "qemu"
#else
    #error "Unknown model type"
#endif


#endif
