#ifndef __ARCH_RISCV_COMMON_INCLUDE_MSR_H__
#define __ARCH_RISCV_COMMON_INCLUDE_MSR_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


/*
 * Generic CSR read and write
 */
#define read_csr(value, which)      \
    __asm__ __volatile__ (          \
        "csrr %[val], " #which ";"  \
        : [val] "=r" (value)        \
        :                           \
    )

#define write_csr(value, which)     \
    __asm__ __volatile__ (          \
        "csrw " #which ", %[val];"  \
        :                           \
        : [val] "r" (value)         \
    )


/*
 * Machine Status Register
 */
struct machine_status_reg {
    union {
        ulong value;

        struct {
            ulong uie   : 1;
            ulong sie   : 1;
            ulong rsv1  : 1;
            ulong mie   : 1;
            ulong upie  : 1;
            ulong spie  : 1;
            ulong rsv2  : 1;
            ulong mpie  : 1;
            ulong spp   : 1;
            ulong rsv3  : 2;
            ulong mpp   : 2;
            ulong fs    : 2;
            ulong xs    : 2;
            ulong mprv  : 1;
            ulong sum   : 1;
            ulong mxr   : 1;
            ulong tvm   : 1;
            ulong tw    : 1;
            ulong tsr   : 1;

#if (defined(ARCH_RISCV64))
            ulong rsv4  : 9;
            ulong uxl   : 2;
            ulong sxl   : 2;
            ulong rsv5  : 27;
            ulong sd    : 1;
#elif (defined(ARCH_RISCV32))
            ulong rsv4  : 8;
            ulong sd    : 1;
#endif
        };
    };
} packed_struct;

#define read_mstatus(value)     read_csr(value, mstatus)
#define write_mstatus(value)    write_csr(value, mstatus)


/*
 * Machine Exception Program Counter
 */
#define read_mepc(value)        read_csr(value, mepc)
#define write_mepc(value)       write_csr(value, mepc)


/*
 * Physical Memory Protection
 */
#define PMP_CFG_MATCH_OFF   0
#define PMP_CFG_MATCH_TOR   1   // Top of range
#define PMP_CFG_MATCH_NA4   2   // Naturally aligned four-byte region
#define PMP_CFG_MATCH_NAPOT 3   // Naturally aligned power-of-two region, >= 8 bytes

struct phys_mem_prot_cfg_reg_field {
    union {
        u8 value;

        struct {
            u8 read     : 1;
            u8 write    : 1;
            u8 exec     : 1;
            u8 match    : 2;
            u8 rsv      : 2;
            u8 locked   : 1;
        };
    };
} packed_struct;

struct phys_mem_prot_cfg_reg {
    union {
        ulong value;

#if (defined(ARCH_RISCV64))
        struct phys_mem_prot_cfg_reg_field fields[8];
#elif (defined(ARCH_RISCV32))
        struct phys_mem_prot_cfg_reg_field fields[4];
#endif
    };
} packed_struct;

struct phys_mem_prot_addr_reg {
    union {
        ulong value;

        struct {
#if (defined(ARCH_RISCV64))
            ulong word_addr : 54;   // [55:2]
            ulong reserved  : 12;
#elif (defined(ARCH_RISCV32))
            ulong word_addr;        // [33:2]
#endif
        };
    };
} packed_struct;


/*
 * Supervisor Address Translation and Protection Register
 */
#define STAP_MODE_BARE  0
#define STAP_MODE_SV32  1
#define STAP_MODE_SV39  8
#define STAP_MODE_SV48  9
#define STAP_MODE_SV57  10
#define STAP_MODE_SV64  11

struct supervisor_addr_trans_prot_reg {
    union {
        ulong value;

        struct {
#if (defined(ARCH_RISCV64))
            ulong pfn   : 44;
            ulong asid  : 16;
            ulong mode  : 4;
#elif (defined(ARCH_RISCV32))
            ulong pfn   : 22;
            ulong asid  : 9;
            ulong mode  : 1;
#endif
        };
    };
} packed_struct;

#define read_satp(value)        read_csr(value, satp)
#define write_satp(value)       write_csr(value, satp)


#endif
