#include "common/include/inttypes.h"
#include "hal/include/int.h"
#include "hal/include/lib.h"


static enum int_seq_state int_seq_table[INT_SEQ_COUNT];
static int_handler_t int_handler_list[INT_SEQ_COUNT];
static int_handler_t default_int_handler = NULL;


int set_default_int_handler(int_handler_t handler)
{
    panic_if(default_int_handler,
             "Not allowed to overwrite existing default int handlers!\n");

    default_int_handler = handler;
    return 1;
}

int set_int_handler(int seq, int_handler_t handler)
{
    panic_if(int_seq_table[seq] == INT_SEQ_ALLOCATED,
             "Not allowed to overwrite existing int handlers!\n");

    int_seq_table[seq] = INT_SEQ_ALLOCATED;
    if (handler) {
        int_handler_list[seq] = handler;
    }

    return 1;
}

int alloc_int_seq(int_handler_t handler)
{
    int i;

    for (i = INT_SEQ_ALLOC_START; i <= INT_SEQ_ALLOC_END; i++) {
        if (INT_SEQ_FREE == int_seq_table[i]) {
            int_seq_table[i] = INT_SEQ_ALLOCATED;
            int_handler_list[i] = handler;
            return i;
        }
    }

    return 0;
}

void free_int_seq(int seq)
{
    panic_if(INT_SEQ_ALLOCATED != int_seq_table[seq],
             "Unable to free a unallocated entry!\n");

    int_seq_table[seq] = INT_SEQ_FREE;
    int_handler_list[seq] = NULL;
}

int_handler_t get_int_handler(int seq)
{
    int_handler_t handler = int_handler_list[seq];
    return handler;
}

int invoke_int_handler(int seq, struct int_context *ictxt, struct kernel_dispatch *kdi)
{
    int_handler_t handler = get_int_handler(seq);
    if (!handler) {
        handler = default_int_handler;
    }

    int handle_type = INT_HANDLE_SIMPLE;
    if (handler) {
        handle_type = handler(ictxt, kdi);
    }

    return handle_type;
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
}
