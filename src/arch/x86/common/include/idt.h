#ifndef __ARCH_X86_COMMON_INCLUDE_IDT_H__
#define __ARCH_X86_COMMON_INCLUDE_IDT_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/abi.h"


struct int_desc_table_entry {
    union {
        u64 value;

        struct {
            u16 offset_low;
            u16 selector;
            union {
                u8 zero;
                struct {
                    u8 ist      : 3;
                    u8 ignored1 : 5;
                };
            };
            union {
                u8 attri;
                struct {
                    u8 type         : 4;
                    u8 storage_gate : 1;    // Should be zero for trap/int gates
                    u8 calling_priv : 2;
                    u8 present      : 1;
                };
            };
            u16 offset_high;
        };
    };

#ifdef ARCH_AMD64
    union {
        u64 value64;

        struct {
            u32 offset64;
            u32 ignored2;
        };
    };
#endif
} packed8_struct;

struct int_desc_table_ptr {
    u16 limit;
    ulong base;
} packed2_struct;


#endif
