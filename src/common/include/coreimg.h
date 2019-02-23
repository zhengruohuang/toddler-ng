#ifndef __COMMON_INCLUDE_COREIMG_H__
#define __COMMON_INCLUDE_COREIMG_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


#define COREIMG_MAGIC 0x70dd1e21


struct coreimg_header {
    u32     magic;
    u32     file_count;
    u32     image_size;
    struct {
        u16 major;
        u16 minor;
        u16 revision;
        u16 release;
    }       version;
    u8      architecture;
    u8      big_endian;
    u8      build_type;
    u8      reserved[1];
    u64     time_stamp;
} packed_struct;

struct coreimg_record {
    u8      file_type;
    u8      load_type;
    u8      compressed;
    u8      reserved;
    u32     start_offset;
    u32     length;
    char    file_name[20];
} packed_struct;

struct coreimg_fat {
    struct coreimg_header   header;
    struct coreimg_record   records[];
} packed_struct;


#endif
