#ifndef __LOADER_INCLUDE_BOOTINFO_H__
#define __LOADER_INCLUDE_BOOTINFO_H__

#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "loader/include/loader.h"
#include "libk/include/memmap.h"
#include "libk/include/coreimg.h"

/*
 * Coreimg
 */
extern void *find_supplied_coreimg(int *size);
extern void init_coreimg();

/*
 * Exec
 */
extern void load_hal_and_kernel();

#endif
