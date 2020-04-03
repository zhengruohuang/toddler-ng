#ifndef __HAL_INCLUDE_MEM_H__
#define __HAL_INCLUDE_MEM_H__


#include "common/include/inttypes.h"
#include "libk/include/memmap.h"


extern ulong pre_palloc(int count);
extern ulong pre_valloc(int count, ulong paddr, int cache);
extern void init_pre_palloc();

extern int hal_map_range(ulong vaddr, ulong paddr, ulong size, int cache);
extern int kernel_map_range(void *page_table, ulong vaddr, ulong paddr,
                            size_t length, int cacheable, int exec, int write,
                            int kernel, int override);
extern ulong hal_translate(ulong vaddr);

extern void init_mem_map();


#endif
