#ifndef __ARCH_ARM_HAL_INCLUDE_SETUP_H__
#define __ARCH_ARM_HAL_INCLUDE_SETUP_H__


#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/mp.h"


/*
 * Int
 */
extern_per_cpu(ulong, cur_int_stack_top);

extern void init_int_entry_mp();
extern void init_int_entry();


/*
 * Switch
 */
extern void switch_to(ulong sched_id, struct reg_context *context,
                      ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb);


/*
 * Map
 */
extern ulong translate(void *page_table, ulong vaddr);
extern int map_range(void *page_table, ulong vaddr, ulong paddr, ulong size,
                     int cache, int exec, int write, int kernel, int override,
                     palloc_t palloc);
extern int unumap_range(void *page_table, ulong vaddr, ulong paddr, ulong size);


#endif
