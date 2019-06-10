#ifndef __ARCH_RISCV_COMMON_INCLUDE_ARCH_H__
#define __ARCH_RISCV_COMMON_INCLUDE_ARCH_H__


// RISC-V 32
#if (defined(ARCH_RISCV32))

#define ARCH_NAME "riscv32"
#define ARCH_WIDTH 32
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

#define HAL_BASE 0xfff88000
#define KERNEL_BASE 0xfff08000

// RISC-V 64
#elif (defined(ARCH_RISCV64))

#define ARCH_NAME "riscv64"
#define ARCH_WIDTH 64
#define ARCH_LITTLE_ENDIAN 1
#define ARCH_BIG_ENDIAN 0

#define HAL_BASE 0xffff88000
#define KERNEL_BASE 0xffff08000

// Unknown
#else

#error "Unknown arch type"

#endif


#endif

