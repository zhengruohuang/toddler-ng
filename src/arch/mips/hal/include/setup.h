#ifndef __ARCH_MIPS_HAL_INCLUDE_SETUP_H__
#define __ARCH_MIPS_HAL_INCLUDE_SETUP_H__


#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/mp.h"


/*
 * Direct access
 */
extern void *cast_paddr_to_cached_seg(paddr_t paddr);
extern ulong cast_paddr_to_uncached_seg(paddr_t paddr);
extern paddr_t cast_cached_seg_to_paddr(void *ptr);
extern paddr_t cast_uncached_seg_to_paddr(ulong vaddr);


/*
 * Int
 */
extern void init_int_entry_mp();
extern void init_int_entry();

extern void restore_context_gpr(struct reg_context *target_ctxt);


/*
 * Switch
 */
extern void switch_to(ulong thread_id, struct reg_context *context,
                      void *page_table, int user_mode, ulong asid, ulong tcb);

extern void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi);
extern void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi);


/*
 * TLB
 */
extern void invalidate_tlb(ulong asid, ulong vaddr, size_t size);
extern void flush_tlb();

extern void init_tlb_mp();
extern void init_tlb();


#endif
