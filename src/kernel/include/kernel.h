#ifndef __KERNEL_INCLUDE_KERNEL__
#define __KERNEL_INCLUDE_KERNEL__


#include "hal/include/export.h"


extern struct hal_exports *get_hal_exports();

extern int hal_get_num_cpus();
extern ulong hal_get_cur_mp_seq();

extern void hal_stop();

extern int hal_disable_local_int();
extern void hal_enable_local_int();
extern int hal_restore_local_int(int enabled);

extern u64 hal_get_ms();

extern void hal_set_syscall_return(struct reg_context *context, int success, ulong return0, ulong return1);


#endif
