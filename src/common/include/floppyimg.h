#ifndef __COMMON_INCLUDE_FLOPPYIMG_H__
#define __COMMON_INCLUDE_FLOPPYIMG_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


struct floppy_fat_header {
    u16 file_count;
    u16 fat_count;

    struct {
        u16 major;
        u16 minor;
        u16 revision;
        u16 release;
    } version;

    u32 create_time;
} packed_struct;

struct floppy_fat_entry {
    u16 start_sector;
    u16 sector_count;
    u8  file_name[12];
} packed_struct;

struct floppy_fat_master {
    struct floppy_fat_header header;
    struct floppy_fat_entry entries[31];
} packed_struct;

struct floppy_fat_slave {
    struct floppy_fat_entry entries[32];
} packed_struct;


#endif
