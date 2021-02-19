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
extern_per_cpu(ulong, cur_int_stack_top);

extern void init_int_entry_mp();
extern void init_int_entry();

extern void restore_context_gpr();


/*
 * Switch
 */
extern_per_cpu(struct page_frame *, cur_page_dir);
extern_per_cpu(ulong, cur_asid);

extern void switch_to(ulong thread_id, struct reg_context *context,
                      void *page_table, int user_mode, ulong asid, ulong tcb);
extern void init_switch_mp();
extern void init_switch();

extern void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi);
extern void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi);


/*
 * Page
 */
extern void *init_user_page_table();
extern void free_user_page_table(void *ptr);

extern paddr_t translate_attri(void *page_table, ulong vaddr,
                               int *exec, int *read, int *write, int *cache);
extern paddr_t translate(void *page_table, ulong vaddr);
extern int map_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size,
                     int cache, int exec, int write, int kernel, int override,
                     palloc_t palloc);
extern int unmap_range(void *page_table, ulong vaddr, paddr_t paddr, ulong size);


/*
 * TLB
 */
extern void invalidate_tlb(ulong asid, ulong vaddr, size_t size);

extern void init_tlb_mp();
extern void init_tlb();


#endif
