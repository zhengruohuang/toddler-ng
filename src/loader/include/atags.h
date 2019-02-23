#ifndef __LOADER_INCLUDE_ATAGS_H__
#define __LOADER_INCLUDE_ATAGS_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


#define ATAG_NONE       0x00000000
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_VIDEOTEXT  0x54410003
#define ATAG_RAMDISK    0x54410004
#define ATAG_INITRD2    0x54420005
#define ATAG_SERIAL     0x54410006
#define ATAG_REVISION   0x54410007
#define ATAG_VIDEOLFB   0x54410008
#define ATAG_CMDLINE    0x54410009


struct atag {
    u32 size;
    u32 tag;

    union {
        // 0x54410001
        struct {
            u32 flags;          // bit 0 = read-only
            u32 pagesize;       // systems page size (usually 4k)
            u32 rootdev;        // root device number
        } core;

        // 0x54410002
        struct {
            u32 size;           // size of the area
            u32 start;          // physical start address
        } mem;

        // 0x54410003
        struct {
            u8 x, y;            // width and height
            u16 page;
            u8 mode, cols;
            u16 ega_bx;
            u8 lines, is_vga;
            u16 points;
        } videotext;

        // 0x54420004
        struct {
            u32 flags;          // bit 0 = load, bit 1 = prompt
            u32 size;           // decompressed ramdisk size in _kilo_ bytes
            u32 start;          // starting block of floppy-based RAM disk image
        } ramdisk;

        // 0x54420005
        struct {
            u32 start;          // physical start address
            u32 size;           // size of compressed ramdisk image in bytes
        } initrd2;

        // 0x54420006
        struct {
            u32 low;
            u32 high;
        } serial;

        // 0x54410007
        u32 rev;

        // 0x54410008
        struct {
            u16 width, height, depth, line_len;
            u32 base, size;
            u8 red_size, red_pos, green_size, green_pos, blue_size, blue_pos;
            u8 rsvd_size, rsvd_pos;
        } videolfb;

        // 0x54410009
        char cmdline[1];        // minimum size
    };
} packed_struct;


#endif
