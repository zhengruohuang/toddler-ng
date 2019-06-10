#ifndef __ARCH_X86_COMMON_INCLUDE_ARCH_H__
#define __ARCH_X86_COMMON_INCLUDE_ARCH_H__


// IA32
#if (defined(ARCH_IA32))

#define ARCH_NAME "ia32"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

#define HAL_BASE 0xfff88000
#define KERNEL_BASE 0xfff08000

// AMD64
#elif (defined(ARCH_AMD64))

#define ARCH_NAME "amd64"
#define ARCH_WIDTH 64
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

#define HAL_BASE 0xffffff88000
#define KERNEL_BASE 0xffffff08000

// Unknown
#else

#error "Unknown arch type"

#endif


#endif
