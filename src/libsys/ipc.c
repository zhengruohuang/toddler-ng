#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/ipc.h"
#include "common/include/mem.h"
#include "libk/include/debug.h"
#include "libk/include/mem.h"
#include "libk/include/string.h"
#include "libsys/include/syscall.h"
#include "libsys/include/ipc.h"


/*
 * Message manipulation
 */
msg_t *get_msg()
{
    thread_info_block_t *tib = syscall_get_tib();
    return tib->msg;
}

void clear_msg(msg_t *msg)
{
    msg->size = 0;
}

void zero_msg(msg_t *msg)
{
    memzero((void *)msg, MAX_MSG_SIZE);
}

void msg_append_param(msg_t *msg, ulong param)
{
    panic_if(msg->has_data, "Param cannot be appended after data!\n");
    msg->params[msg->num_params++] = param;
}

void *msg_append_data(msg_t *msg, void *data, ulong size)
{
    panic_if(msg->data_bytes + size > MAX_MSG_DATA_SIZE, "Data too large to fit in msg!\n");

    ulong cur_data_bytes = msg->data_bytes;
    void *msg_data_start = (void *)&msg->params[msg->num_params];

    void *msg_data = msg_data_start + cur_data_bytes;
    if (data) {
        memcpy(msg_data, data, size);
    }

    msg->has_data = 1;
    msg->data_bytes += size;

    return msg_data_start;
}

ulong msg_get_param(msg_t *msg, int idx)
{
    panic_if(idx >= msg->num_params, "Bad param index!\n");
    return msg->params[idx];
}

void *msg_get_data(msg_t *msg, ulong *size)
{
    void *data = (void *)&msg->params[msg->num_params];
    if (size) {
        *size = msg->has_data ? msg->data_bytes : 0;
    }
    return data;
}
