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

extern void restore_context_gpr();


/*
 * Switch
 */
extern void switch_to(ulong thread_id, struct reg_context *context,
                      void *page_table, int user_mode, ulong asid, ulong tcb);

extern void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi);
extern void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi);


/*
 * Map
 */
extern paddr_t translate(void *page_table, ulong vaddr);
extern int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write, int kernel, int override,
                     palloc_t palloc);
extern int unumap_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size);


#endif
