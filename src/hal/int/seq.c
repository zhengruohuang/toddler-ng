#include "common/include/inttypes.h"
#include "hal/include/int.h"
#include "hal/include/lib.h"


static enum int_seq_state int_seq_table[INT_SEQ_COUNT];
static int_handler_t int_handler_list[INT_SEQ_COUNT];


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
    return handler ? handler(ictxt, kdi) : INT_HANDLE_SIMPLE;
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
