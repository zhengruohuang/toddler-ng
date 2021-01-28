#ifndef __LIBC_INCLUDE_SYS_H__
#define __LIBC_INCLUDE_SYS_H__


/*
 * Futex
 */
#include <futex.h>


/*
 * Syscall
 */
#include "libsys/include/syscall.h"


/*
 * IPC
 */
#include "libsys/include/ipc.h"

#define INVALID_MSG_OPCODE  (-1ul)
#define MSG_OPCODE_MASK     (0xFFFFul)
#define GLOBAL_MSG_PORT     (MSG_OPCODE_MASK)

typedef ulong (*msg_handler_t)(ulong opcode);

extern void init_ipc();
extern int register_msg_handler(ulong port, msg_handler_t handler);
extern int cancel_msg_handler(ulong port);


#endif
