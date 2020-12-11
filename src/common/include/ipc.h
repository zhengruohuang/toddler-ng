#ifndef __COMMON_INCLUDE_IPC__
#define __COMMON_INCLUDE_IPC__


#include "common/include/inttypes.h"
#include "common/include/compiler.h"
#include "common/include/mem.h"


enum ipc_flags {
    IPC_SEND_SERIAL = 0x1,
    IPC_SEND_POPUP  = 0x2,
    IPC_SEND_SERIAL_POPUP   = IPC_SEND_SERIAL | IPC_SEND_POPUP,

    IPC_SEND_WAIT_FOR_REPLY = 0x4,
};


#define MAX_MSG_SIZE        2048
#define MAX_MSG_DATA_SIZE   (MAX_MSG_SIZE - sizeof(ulong))


typedef volatile struct {
    union {
        ulong size;
        struct {
            ulong num_params : 9;
            ulong has_data   : 1;
            ulong data_bytes : 20;
        };
    };
    ulong params[MAX_MSG_DATA_SIZE / sizeof(ulong)];
} natural_struct msg_t;


#endif
