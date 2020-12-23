#ifndef __HAL_INCLUDE_INT_H__
#define __HAL_INCLUDE_INT_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"
#include "common/include/context.h"
#include "hal/include/dispatch.h"
#include "hal/include/mp.h"


/*
 * Handling type
 */
enum int_handling_type {
    INT_HANDLE_SIMPLE       = 0x00,

    INT_HANDLE_STAY_HAL     = 0x00,
    INT_HANDLE_CALL_KERNEL  = 0x01,

    INT_HANDLE_AUTO_UNMASK  = 0x00,
    INT_HANDLE_KEEP_MASKED  = 0x10,

};


/*
 * Interrupt handler
 */
struct int_context {
    ulong vector;
    ulong error_code;

    struct reg_context *regs;

    ulong mp_seq;
    void *param;
} natural_struct;

typedef int (*int_handler_t)(struct int_context *ictxt, struct kernel_dispatch *kdi);

extern void int_handler(int seq, struct int_context *ictxt);
extern void init_int_handler();


/*
 * Interrupt seqs
 */
#define INT_SEQ_INVALID     0
#define INT_SEQ_PANIC       123
#define INT_SEQ_PAGE_FAULT  124
#define INT_SEQ_DUMMY       125
#define INT_SEQ_DEV         126
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
extern int invoke_int_handler(int seq, struct int_context *ictxt, struct kernel_dispatch *kdi);


/*
 * Syscall
 */
extern void init_syscall();


/*
 * Int state
 */
extern int get_local_int_state();
extern void set_local_int_state(int enabled);

extern int disable_local_int();
extern void enable_local_int();
extern int restore_local_int(int enabled);

extern void init_int_state();
extern void init_int_state_mp();


/*
 * Context
 */
extern_per_cpu(ulong, cur_running_thread_id);
extern_per_cpu(int, cur_in_user_mode);
extern_per_cpu(struct reg_context, cur_context);
extern_per_cpu(ulong, cur_tcb_vaddr);

extern void switch_context(ulong thread_id, struct reg_context *context,
                           void *page_table, int user_mode, ulong asid, ulong tcb);

extern void init_context();
extern void init_context_mp();


#endif
