#ifndef __HAL_INCLUDE_INT_H__
#define __HAL_INCLUDE_INT_H__


#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "hal/include/dispatch.h"
#include "hal/include/mp.h"


/*
 * Handling type
 */
enum int_handling_type {
    INT_HANDLE_TYPE_TAKEOVER,
    INT_HANDLE_TYPE_HAL,
    INT_HANDLE_TYPE_KERNEL,
};


/*
 * Interrupt handler
 */
struct int_context {
    ulong vector;
    ulong error_code;

    struct reg_context *regs;
};

typedef int (*int_handler_t)(struct int_context *ictxt, struct kernel_dispatch_info *kdi);


/*
 * Interrupt seqs
 */
#define INT_SEQ_DUMMY       126
#define INT_SEQ_SYSCALL     127
#define INT_SEQ_ALLOC_START 128
#define INT_SEQ_ALLOC_END   255
#define INT_SEQ_COUNT       (INT_SEQ_ALLOC_END + 1)

enum int_seq_state {
    INT_SEQ_UNKNOWN,
    INT_SEQ_RESERVED,
    INT_SEQ_FREE,
    INT_SEQ_ALLOCATED,
    INT_SEQ_OTHER,
};

extern void init_int_seq();
extern int set_int_handler(int seq, int_handler_t hdlr);
extern int alloc_int_seq(int_handler_t hdlr);
extern void free_int_seq(int seq);
extern int_handler_t get_int_handler(int seq);


/*
 * Syscall
 */
extern void init_syscall();


/*
 * Int state
 */
int get_local_int_state();
void set_local_int_state(int enabled);
int disable_local_int();
void enable_local_int();
int restore_local_int(int enabled);
void init_int_state();


/*
 * Context
 */
extern_per_cpu(ulong, cur_running_sched_id);
extern_per_cpu(int, cur_in_user_mode);
extern_per_cpu(struct reg_context, cur_context);
extern_per_cpu(ulong, cur_tcb_vaddr);

extern void switch_context(ulong sched_id, struct reg_context *context,
                    ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb);
extern void init_context();



#endif
