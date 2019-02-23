#ifndef __COMMON_INCLUDE_INITIMG_H__
#define __COMMON_INCLUDE_INITIMG_H__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"


#define INITIMG_MAGIC   0x70dd1e2


struct initimg_header {
    u32 magic;

    /*
     * Size including header and all other components and paddings
     */
    u32 size;

    /*
     * 0 if the component is not present
     * Otherwise the offset to the component from the beginning of bootimg_info
     */
    u32 fdt_offset;
    u32 coreimg_offset;
} packed_struct;


#endif
