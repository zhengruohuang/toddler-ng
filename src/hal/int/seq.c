#include "common/include/inttypes.h"
#include "hal/include/int.h"
#include "hal/include/lib.h"


static enum int_seq_state int_seq_table[INT_SEQ_COUNT];
static int_handler_t int_handler_list[INT_SEQ_COUNT];


/*
 * Default dummy handler
 */
static int int_handler_dummy(struct int_context *context, struct kernel_dispatch_info *kdi)
{
//     kprintf("Interrupt, Vector: %lx, PC: %x, SP: %x, CPSR: %x\n",
//             context->vector, context->context->pc, context->context->sp, context->context->cpsr);

    panic("Unregistered interrupt @ %p", (void *)(ulong)context->vector);
    return INT_HANDLE_TYPE_HAL;
}


int set_int_handler(int seq, int_handler_t hdlr)
{
    assert(int_seq_table[seq] == INT_SEQ_FREE || int_seq_table[seq] == INT_SEQ_RESERVED);

    int_seq_table[seq] = INT_SEQ_ALLOCATED;

    if (hdlr) {
        int_handler_list[seq] = hdlr;
    }

    return 1;
}

int alloc_int_seq(int_handler_t hdlr)
{
    int i;

    for (i = INT_SEQ_ALLOC_START; i <= INT_SEQ_ALLOC_END; i++) {
        if (INT_SEQ_FREE == int_seq_table[i]) {
            int_seq_table[i] = INT_SEQ_ALLOCATED;

            if (NULL != hdlr) {
                int_handler_list[i] = hdlr;
            }

            return i;
        }
    }

    return 0;
}

void free_int_seq(int seq)
{
    assert(INT_SEQ_ALLOCATED == int_seq_table[seq]);

    int_seq_table[seq] = INT_SEQ_FREE;
    int_handler_list[seq] = NULL;
}

int_handler_t get_int_handler(int seq)
{
    int_handler_t handler = int_handler_list[seq];
    if (!handler) {
        handler = int_handler_dummy;
    }

    return handler;
}

void init_int_seq()
{
    int i;

    // Reserved
    for (i = 0; i < INT_SEQ_ALLOC_START; i++) {
        int_seq_table[i] = INT_SEQ_RESERVED;
    }

    // Allocatable
    for (i = INT_SEQ_ALLOC_START; i <= INT_SEQ_ALLOC_END; i++) {
        int_seq_table[i] = INT_SEQ_FREE;
    }

    // Register the dummy handler
    set_int_handler(INT_SEQ_DUMMY, int_handler_dummy);
}
