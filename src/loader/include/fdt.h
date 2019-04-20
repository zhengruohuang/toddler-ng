#ifndef __COMMON_INCLUDE_FDT_H__
#define __COMMON_INCLUDE_FDT_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


#define FDT_HEADER_MAGIC    0xd00dfeed

struct fdt_header {
    u32 magic;
    u32 total_size;
    u32 off_dt_struct;
    u32 off_dt_strings;
    u32 off_mem_rsvmap;
    u32 version;
    u32 last_comp_version;
    u32 boot_cpuid_phys;
    u32 size_dt_strings;
    u32 size_dt_struct;
} packed_struct;

struct fdt_memrsv_entry {
    u64 addr;
    u64 size;
} packed_struct;

enum fdt_struct_token {
    FDT_BEGIN_NODE = 0x1,
    FDT_END_NODE = 0x2,
    FDT_PROP = 0x3,
    FDT_NOP = 0x4,
    FDT_END = 0x9,
};

struct fdt_struct {
    u32 token;
} packed_struct;

struct fdt_struct_begin {
    u32 token;
    char name[];
} packed_struct;

struct fdt_struct_prop {
    u32 token;
    u32 len;
    u32 off_name;
} packed_struct;


#endif
