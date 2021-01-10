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
#define MAX_MSG_PARAMS      15
#define MAX_MSG_DATA_WORDS  (MAX_MSG_SIZE / sizeof(ulong) - 3 - MAX_MSG_PARAMS)


typedef volatile struct {
    union {
        ulong size;
        struct {
            ulong param_type_map    : 16;   // bitmap[1 << idx] = data ? 1 : 0
            ulong num_params        : 4;
            ulong num_data_words    : 12;
        };
    };

    struct {
        ulong pid;
        ulong tid;
    } sender;

    ulong params[MAX_MSG_PARAMS];
    ulong data[MAX_MSG_DATA_WORDS];
} natural_struct msg_t;


#endif
