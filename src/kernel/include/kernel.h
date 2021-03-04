#ifndef __KERNEL_INCLUDE_KERNEL__
#define __KERNEL_INCLUDE_KERNEL__


#include "hal/include/export.h"


#ifndef decl_per_cpu
#define decl_per_cpu(type, name)    int __##name##_per_cpu_offset = -1
#endif

#ifndef extern_per_cpu
#define extern_per_cpu(type, name)  extern int __##name##_per_cpu_offset
#endif

#ifndef get_per_cpu
#define get_per_cpu(type, name)     ((type *)hal_access_per_cpu_var(&__##name##_per_cpu_offset, sizeof(type)))
#endif


extern struct hal_exports *get_hal_exports();

extern int hal_get_num_cpus();
extern ulong hal_get_cur_mp_seq();

extern void hal_idle();
extern void hal_stop();

extern int hal_disable_local_int();
extern void hal_enable_local_int();
extern int hal_restore_local_int(int enabled);

extern u64 hal_get_ms();

extern ulong hal_get_asid_limit();
extern ulong hal_get_vaddr_base();
extern ulong hal_get_vaddr_limit();

extern void hal_set_syscall_return(struct reg_context *context, int success, ulong return0, ulong return1);

extern void *hal_access_per_cpu_var(int *offset, size_t size);

extern void *hal_cast_paddr_to_kernel_ptr(paddr_t paddr);
extern paddr_t hal_cast_kernel_ptr_to_paddr(void *ptr);


#endif
