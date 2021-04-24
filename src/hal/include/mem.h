#ifndef __HAL_INCLUDE_MEM_H__
#define __HAL_INCLUDE_MEM_H__


#include "common/include/inttypes.h"
#include "libk/include/memmap.h"
#include "libk/include/mem.h"


/*
 * Sysare and Pre-palloc
 */
extern ulong get_sysarea_range(ulong *lower, ulong *upper);

extern ppfn_t pre_palloc(int count);
extern ulong pre_valloc(int count, paddr_t paddr, int cache, int kernel);

extern void init_pre_palloc();
extern void disable_pre_palloc();


/*
 * Mapping
 */
extern int hal_map_range(ulong vaddr, paddr_t paddr, ulong size, int cache, int user);
extern paddr_t hal_translate(ulong vaddr);

extern void init_mem_map();


#endif
