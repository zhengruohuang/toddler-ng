#ifndef __HAL_INCLUDE_KERNEL__
#define __HAL_INCLUDE_KERNEL__


#include "common/include/inttypes.h"
#include "hal/include/export.h"


/*
 * Kernel exports
 */
ulong kernel_palloc_tag(int count, int tag);
ulong kernel_palloc(int count);
int kernel_pfree(ulong pfn);
void kernel_dispatch(ulong sched_id, struct kernel_dispatch_info *kdi);


/*
 * Memory zone
 */
extern struct hal_memmap_entry *init_kmem_zone(int *count, u64 *paddr_end);


/*
 * General kernel
 */
extern void init_kernel();


#endif
