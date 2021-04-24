#ifndef __ARCH_X86_HAL_INCLUDE_SETUP_H__
#define __ARCH_X86_HAL_INCLUDE_SETUP_H__


#include "common/include/inttypes.h"
#include "hal/include/hal.h"


/*
 * Int
 */
extern void init_int_entry_mp();
extern void init_int_entry();

extern void restore_context_gpr(struct reg_context *regs, int user_mode);


/*
 * Switch
 */
extern void switch_to(ulong thread_id, struct reg_context *context,
                      void *page_table, int user_mode, ulong asid, ulong tcb);

extern void kernel_pre_dispatch(ulong thread_id, struct kernel_dispatch *kdi);
extern void kernel_post_dispatch(ulong thread_id, struct kernel_dispatch *kdi);


/*
 * Mem
 */
extern void *get_kernel_page_table();
extern void switch_page_table(void *page_table, ulong asid);

extern void invalidate_tlb(ulong asid, ulong vaddr, size_t size);
extern void flush_tlb();

extern void init_mmu_mp();
extern void init_mmu();


/*
 * MP
 */
typedef ulong (*get_apic_id_t)();

extern ulong get_cur_mp_id();
extern int get_cur_mp_seq();

extern void setup_mp_id(get_apic_id_t f);
extern void setup_mp_seq();

extern void init_mp_boot();


/*
 * Segment
 */
extern u16 get_segment_selector(char type, int user_mode);
extern u16 get_kernel_code_selector();

extern void set_user_tls_ptr(int mp_seq, ulong tls);

extern void load_tss_mp(int mp_seq, ulong sp);
extern void load_gdt_mp(int mp_seq);

extern void load_kernel_seg();
extern void load_program_seg(ulong es, ulong fs, ulong gs);

extern void init_segment();


#endif
