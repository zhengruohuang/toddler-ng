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
 * Status
 */
struct status_reg {
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
} natural_struct;

#define read_mstatus(value)     read_csr(value, mstatus)
#define write_mstatus(value)    write_csr(value, mstatus)

#define read_sstatus(value)     read_csr(value, sstatus)
#define write_sstatus(value)    write_csr(value, sstatus)


/*
 * Exception
 */
#define TVEC_MODE_DIRECT    0
#define TVEC_MODE_VECTORED  1

struct trap_vec_base_addr_reg {
    union {
        ulong value;

        struct {
            ulong mode          : 2;
            ulong base2         : sizeof(ulong) * 8 - 2; // base2 = base >> 2
        };
    };
} natural_struct;

struct cause_reg {
    union {
        ulong value;

        struct {
            ulong except_code   : sizeof(ulong) * 8 - 1;
            ulong interrupt     : 1;
        };
    };
} natural_struct;

#define read_mepc(value)        read_csr(value, mepc)
#define write_mepc(value)       write_csr(value, mepc)

#define read_sscratch(value)    read_csr(value, sscratch)
#define write_sscratch(value)   write_csr(value, sscratch)

#define read_sepc(value)        read_csr(value, sepc)
#define write_sepc(value)       write_csr(value, sepc)

#define read_scause(value)      read_csr(value, scause)
#define write_scause(value)     write_csr(value, scause)

#define read_stval(value)       read_csr(value, stval)
#define write_stval(value)      write_csr(value, stval)

#define read_stvec(value)       read_csr(value, stvec)
#define write_stvec(value)      write_csr(value, stvec)


/*
 * Interrupt
 */
#define read_sedeleg(value)     read_csr(value, sedeleg)
#define write_sedeleg(value)    write_csr(value, sedeleg)

#define read_sie(value)         read_csr(value, sie)
#define write_sie(value)        write_csr(value, sie)

#define read_sip(value)         read_csr(value, sip)
#define write_sip(value)        write_csr(value, sip)


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
} natural_struct;

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
} natural_struct;


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
