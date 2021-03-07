#ifndef __HAL_INCLUDE_MP_H__
#define __HAL_INCLUDE_MP_H__


#include "common/include/inttypes.h"
#include "common/include/mem.h"


/*
 * Per-CPU var
 */
#ifndef decl_per_cpu
#define decl_per_cpu(type, name)    int __##name##_per_cpu_offset = -1
#endif

#ifndef extern_per_cpu
#define extern_per_cpu(type, name)  extern int __##name##_per_cpu_offset
#endif

#ifndef get_per_cpu
#define get_per_cpu(type, name)     ((type *)access_per_cpu_var(&__##name##_per_cpu_offset, sizeof(type)))
#endif

#ifndef get_per_cpu_val
#define get_per_cpu_val(type, name) (*(type *)access_per_cpu_var(&__##name##_per_cpu_offset, sizeof(type)))
#endif

extern ulong get_my_cpu_area_start_vaddr();
extern ulong get_my_cpu_data_area_start_vaddr();
extern ulong get_my_cpu_stack_top_vaddr();
extern ulong get_my_cpu_init_stack_top_vaddr();

extern void *access_per_cpu_var(int *offset, size_t size);
extern void init_per_cpu_area();


/*
 * Topology
 */
extern int get_num_cpus();

extern void setup_mp_id_trans(int cpus, int fields, ...);
extern void add_mp_id_trans(ulong mp_id);
extern void final_mp_id_trans();

extern ulong get_mp_id_by_seq(int mp_seq);
extern int get_mp_seq_by_id(ulong mp_id);
extern int get_cur_mp_seq();

extern void init_topo();


/*
 * Secondary CPUs
 */
extern int is_single_cpu();

extern void bringup_all_secondary_cpus();
extern void release_secondary_cpu_lock();
extern void secondary_cpu_init_done();


/*
 * Halt
 */
extern void halt_all_cpus();
extern void handle_halt();


#endif
