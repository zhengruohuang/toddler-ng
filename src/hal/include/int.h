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

    ulong mp_id;
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
extern int set_default_int_handler(int_handler_t handler);
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
struct running_context {
    struct reg_context *kernel_context;

    ulong thread_id;
    void *page_table;
    int user_mode;
    ulong asid;
    ulong tcb;
};

extern_per_cpu(struct reg_context *, cur_int_reg_context);
extern_per_cpu(struct running_context, cur_running_context);

static inline struct running_context *get_cur_running_context()
{
    return get_per_cpu(struct running_context, cur_running_context);
}

static inline ulong get_cur_running_thread_id()
{
    struct running_context *rctxt = get_cur_running_context();
    return rctxt->thread_id;
}

static inline int get_cur_running_in_user_mode()
{
    struct running_context *rctxt = get_cur_running_context();
    return rctxt->user_mode;
}

static inline void set_cur_running_in_user_mode(int user_mode)
{
    struct running_context *rctxt = get_cur_running_context();
    rctxt->user_mode = user_mode;
}

static inline ulong get_cur_running_tcb()
{
    struct running_context *rctxt = get_cur_running_context();
    return rctxt->tcb;
}

static inline ulong get_cur_running_asid()
{
    struct running_context *rctxt = get_cur_running_context();
    return rctxt->asid;
}

static inline void *get_cur_running_page_table()
{
    struct running_context *rctxt = get_cur_running_context();
    return rctxt->page_table;
}

static inline struct reg_context *get_cur_int_reg_context()
{
    return *get_per_cpu(struct reg_context *, cur_int_reg_context);
}

extern void switch_context(ulong thread_id, struct reg_context *context,
                           void *page_table, int user_mode, ulong asid, ulong tcb);

extern void init_context();
extern void init_context_mp();


#endif
