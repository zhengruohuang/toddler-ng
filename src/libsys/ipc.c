#include "common/include/inttypes.h"
#include "common/include/abi.h"
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

msg_t *get_empty_msg()
{
    msg_t *msg = get_msg();
    msg->size = 0;
    return msg;
}

msg_t *get_response_msg()
{
    msg_t *msg = get_msg();
    atomic_mb();
    return msg;
}

void clear_msg(msg_t *msg)
{
    msg->size = 0;
}

void zero_msg(msg_t *msg)
{
    memzero((void *)msg, MAX_MSG_SIZE);
}


/*
 * Message info
 */
size_t msg_remain_data_size(msg_t *msg)
{
    return (MAX_MSG_DATA_WORDS - msg->num_data_words) * sizeof(ulong);
}


/*
 * Append param
 */
void msg_append_param(msg_t *msg, ulong param)
{
    panic_if(msg->num_params >= MAX_MSG_PARAMS, "Too many params in msg!\n");
    msg->params[msg->num_params++] = param;
}

void msg_append_int(msg_t *msg, int param)
{
    panic_if(msg->num_params >= MAX_MSG_PARAMS, "Too many params in msg!\n");
    msg->params[msg->num_params++] = (ulong)(long)param;
}

void *msg_append_data(msg_t *msg, const void *data, size_t size)
{
    panic_if(msg->num_params >= MAX_MSG_PARAMS, "Too many params in msg!\n");

    size_t new_data_words = align_up_vsize(size, sizeof(ulong)) / sizeof(ulong);
    panic_if(msg->num_data_words + new_data_words > MAX_MSG_DATA_WORDS,
             "Data does not fit in msg!\n");

    void *data_start = (void *)&msg->data[msg->num_data_words];
    if (data) {
        memcpy(data_start, data, size);
    }

    msg->params[msg->num_params] = (msg->num_data_words << 16) | new_data_words;
    msg->param_type_map |= 0x1 << msg->num_params;
    msg->num_data_words += new_data_words;
    msg->num_params++;

    return data_start;
}

void *msg_try_append_data(msg_t *msg, const void *data, size_t size)
{
    if (msg->num_params >= MAX_MSG_PARAMS) {
        return NULL;
    }

    size_t new_data_words = align_up_vsize(size, sizeof(ulong)) / sizeof(ulong);
    if (msg->num_data_words + new_data_words > MAX_MSG_DATA_WORDS) {
        return NULL;
    }

    void *data_start = (void *)&msg->data[msg->num_data_words];
    if (data) {
        memcpy(data_start, data, size);
    }

    msg->params[msg->num_params] = (msg->num_data_words << 16) | new_data_words;
    msg->param_type_map |= 0x1 << msg->num_params;
    msg->num_data_words += new_data_words;
    msg->num_params++;

    return data_start;
}

char *msg_append_str(msg_t *msg, const char *str, size_t size)
{
    panic_if(msg->num_params >= MAX_MSG_PARAMS, "Too many params in msg!\n");

    if (!size) {
        size = strlen(str);
    }
    size_t new_data_words = align_up_vsize(size + 1, sizeof(ulong)) / sizeof(ulong);
    panic_if(msg->num_data_words + new_data_words > MAX_MSG_DATA_WORDS,
             "Data does not fit in msg!\n");

    char *data_start = (void *)&msg->data[msg->num_data_words];
    if (str) {
        strncpy(data_start, str, size + 1);
        data_start[size] = '\0';
        //kprintf("str: %s\n", data_start);
    }

    msg->params[msg->num_params] = (msg->num_data_words << 16) | new_data_words;
    msg->param_type_map |= 0x1 << msg->num_params;
    msg->num_data_words += new_data_words;
    msg->num_params++;

    return data_start;
}


/*
 * Get param
 */
ulong msg_get_param(msg_t *msg, int idx)
{
    panic_if(idx >= msg->num_params, "Bad param index: %d, num params: %lu\n",
             idx, msg->num_params);
    panic_if(msg->param_type_map & (0x1 << idx), "Bad param type!\n");
    return msg->params[idx];
}

int msg_get_int(msg_t *msg, int idx)
{
    ulong param = msg_get_param(msg, idx);
    return (int)(long)param;
}

void *msg_get_data(msg_t *msg, int idx, size_t *size)
{
    panic_if(idx >= msg->num_params, "Bad param index!\n");
    panic_if(!(msg->param_type_map & (0x1 << idx)), "Bad param type!\n");

    ulong data_word_idx = msg->params[idx] >> 16;
    size_t data_words = msg->params[idx] & 0xfffful;

    if (size) {
        *size = data_words * sizeof(ulong);
    }
    return (void *)&msg->data[data_word_idx];
}

void msg_copy_data(msg_t *msg, int idx, void *buf, size_t size)
{
    panic_if(idx >= msg->num_params, "Bad param index!\n");
    panic_if(!(msg->param_type_map & (0x1 << idx)), "Bad param type!\n");

    ulong data_word_idx = msg->params[idx] >> 16;
    size_t data_words = msg->params[idx] & 0xfffful;

    void *data = (void *)&msg->data[data_word_idx];
    size_t data_size = data_words * sizeof(ulong);

    if (buf) {
        size_t copy_size = size > data_size ? data_size : size;
        memcpy(buf, data, copy_size);
    }
}


/*
 * Set param
 */
void msg_set_param(msg_t *msg, int idx, ulong val)
{
    panic_if(idx >= msg->num_params, "Bad param index: %d, num params: %lu\n",
             idx, msg->num_params);
    panic_if(msg->param_type_map & (0x1 << idx), "Bad param type!\n");
    msg->params[idx] = val;
}

void msg_set_int(msg_t *msg, int idx, int val)
{
    msg_set_param(msg, idx, (ulong)(long)val);
}


/*
 * u64
 */
void msg_append_param_u64(msg_t *msg, u64 param)
{
#if (ARCH_WIDTH == 32)
    msg_append_data(msg, &param, sizeof(u64));
#elif (ARCH_WIDTH == 64)
    msg_append_param(msg, (ulong)param);
#else
#error "Unsupported ARCH_WIDTH"
#endif
}

u64 msg_get_param_u64(msg_t *msg, int idx)
{
    u64 value = 0;
#if (ARCH_WIDTH == 32)
    msg_copy_data(msg, idx, &value, sizeof(u64));
#elif (ARCH_WIDTH == 64)
    value = msg_get_param(msg, idx);
#else
#error "Unsupported ARCH_WIDTH"
#endif
    return value;
}
