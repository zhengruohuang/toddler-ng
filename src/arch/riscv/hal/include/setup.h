#ifndef __ARCH_RISCV_HAL_INCLUDE_SETUP_H__
#define __ARCH_RISCV_HAL_INCLUDE_SETUP_H__


#include "common/include/inttypes.h"
#include "hal/include/hal.h"
#include "hal/include/mp.h"


/*
 * Int
 */
extern_per_cpu(ulong, int_stack_top);

extern void init_int_entry_mp();
extern void init_int_entry();

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
extern void switch_page_table(void *page_table, ulong asid);

extern void invalidate_tlb(ulong asid, ulong vaddr, size_t size);
extern void flush_tlb();

extern void init_mmu_mp();
extern void init_mmu();


#endif
