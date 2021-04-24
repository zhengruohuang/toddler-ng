#ifndef __ARCH_X86_COMMON_INCLUDE_GDT_H__
#define __ARCH_X86_COMMON_INCLUDE_GDT_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


struct global_desc_table_entry {
    union {
        u64 value;

        struct {
            u16 limit_low;
            u16 base_low;
            u8 base_mid;

            union {
                u8 access_byte;

                struct {
                    u8 accessed     : 1;
                    u8 read_write   : 1;    // For code: if read is allowed; For data: if write is allowed
                    u8 direct_conform : 1;  // For data: 0 = grow up, 1 = grow down;
                                            // For code: 0 = exec in same priv only, 1 = exec from equal or lower priv
                    u8 exec         : 1;    // 1 = Code, 0 = Data
                    u8 type         : 1;    // 1 = Code/Data, 0 = System
                    u8 privilege    : 2;    // 0 = Kernel, 3 = User
                    u8 present      : 1;
                };
            };

            union {
                u8 flags;

                struct {
                    u8 limit_high   : 4;
#ifdef ARCH_AMD64
                    u8 system       : 1;
                    u8 amd64_seg    : 1;    // For 64-bit code seg only
#else
                    u8 zero         : 2;
#endif
                    u8 size         : 1;    // 0 = 16-bit/64-bit, 1 = 32-bit
                    u8 granularity  : 1;    // 0 = 1B, 1 = 4KB
                };
            };

            u8 base_high;
        };

#ifdef ARCH_AMD64
        struct {
            u32 high32;
            u32 zero;
        } tss64_base;
#endif
    };
} natural_struct;

struct global_desc_table_ptr {
    u16 limit;
    ulong base;
} packed2_struct;

struct segment_selector {
    union {
        u16 value;

        struct {
            u16 privilege   : 2;
            u16 is_ldt      : 1;
            u16 index       : 13;
        };
    };
} packed2_struct;


#endif
