#ifndef __LIBSYS_INCLUDE_IPC__
#define __LIBSYS_INCLUDE_IPC__


#include "common/include/inttypes.h"
#include "common/include/ipc.h"


extern msg_t *get_msg();
extern msg_t *get_empty_msg();
extern msg_t *get_response_msg();
extern void clear_msg(msg_t *msg);
extern void zero_msg(msg_t *msg);

extern size_t msg_remain_data_size(msg_t *msg);

extern void msg_append_param(msg_t *msg, ulong param);
extern void msg_append_int(msg_t *msg, int param);
extern void *msg_append_data(msg_t *msg, void *data, size_t size);
extern void *msg_try_append_data(msg_t *msg, void *data, size_t size);
extern char *msg_append_str(msg_t *msg, const char *str, size_t size);

extern ulong msg_get_param(msg_t *msg, int idx);
extern int msg_get_int(msg_t *msg, int idx);
extern void *msg_get_data(msg_t *msg, int idx, size_t *size);
extern void msg_copy_data(msg_t *msg, int idx, void *buf, size_t size);

extern void msg_set_param(msg_t *msg, int idx, ulong val);
extern void msg_set_int(msg_t *msg, int idx, int val);


#endif
