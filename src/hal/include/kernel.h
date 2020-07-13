#ifndef __HAL_INCLUDE_KERNEL__
#define __HAL_INCLUDE_KERNEL__


#include "common/include/inttypes.h"
#include "common/include/mem.h"
#include "hal/include/export.h"


/*
 * Kernel exports
 */
extern ppfn_t kernel_palloc(int count);
extern int kernel_pfree(ppfn_t ppfn);

extern void *kernel_palloc_ptr(int count);
extern int kernel_pfree_ptr(void *ptr);

extern void kernel_dispatch(ulong sched_id, struct kernel_dispatch *kdi);
extern void kernel_start();

extern void kernel_test_phase1();


/*
 * General kernel
 */
extern void init_kernel();


#endif
