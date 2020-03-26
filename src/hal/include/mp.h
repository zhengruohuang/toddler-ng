#ifndef __HAL_INCLUDE_MP_H__
#define __HAL_INCLUDE_MP_H__


#include "common/include/inttypes.h"
#include "common/include/mem.h"


/*
 * Per-CPU var
 */
#define PER_CPU_AREA_PAGE_COUNT     1
#define PER_CPU_AREA_SIZE           PAGE_SIZE
#define PER_CPU_DATA_START_OFFSET   (PAGE_SIZE >> 1)
#define PER_CPU_STACK_TOP_OFFSET    ((PAGE_SIZE >> 1) - 16)

#ifndef decl_per_cpu
#define decl_per_cpu(type, name)    int __##name##_per_cpu_offset = -1
#endif

#ifndef extern_per_cpu
#define extern_per_cpu(type, name)  extern int __##name##_per_cpu_offset
#endif

#ifndef get_per_cpu
#define get_per_cpu(type, name)     ((type *)access_per_cpu_var(&__##name##_per_cpu_offset, sizeof(type)))
#endif

extern ulong get_my_cpu_area_start_vaddr();
extern void *access_per_cpu_var(int *offset, size_t size);
extern void init_per_cpu_area();


/*
 * Topology
 */
extern int get_num_cpus();
extern void init_topo();


/*
 * Secondary CPUs
 */
extern void bringup_all_secondary_cpus();
extern void release_secondary_cpu_lock();
extern void secondary_cpu_init_done();


/*
 * Halt
 */
extern void halt_all_cpus(int count, ...);
extern void init_halt();


#endif
