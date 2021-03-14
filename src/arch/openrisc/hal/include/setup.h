#ifndef __ARCH_OPENRISC_HAL_INCLUDE_SETUP_H__
#define __ARCH_OPENRISC_HAL_INCLUDE_SETUP_H__


#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/mp.h"


/*
 * Int
 */
extern void init_int_entry_mp();
extern void init_int_entry();

extern void restore_context_gpr_from_shadow1();
extern void restore_context_gpr(struct reg_context *regs);


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
extern void *get_kernel_page_table();
extern void set_page_table(void *page_table);

extern int tlb_refill(int itlb, ulong vaddr);

extern void invalidate_tlb(ulong asid, ulong vaddr, size_t size);
extern void flush_tlb();

extern int disable_mmu();
extern void enable_mmu();

extern void init_mmu_mp();
extern void init_mmu();


/*
 * PMU
 */
extern void pmu_idle();
extern void pmu_halt();

extern void init_pmu();
extern void init_pmu_mp();


#endif
