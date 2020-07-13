#ifndef __COMMON_INCLUDE_SYSCALL__
#define __COMMON_INCLUDE_SYSCALL__


#include "common/include/inttypes.h"


/*
 * System and testing
 */
#define SYSCALL_NONE                0x0
#define SYSCALL_PING                0x1
#define SYSCALL_YIELD               0x2
#define SYSCALL_ILLEGAL             0x3
#define SYSCALL_PUTS                0x4
#define SYSCALL_INTERRUPT           0x5
#define SYSCALL_THREAD_EXIT         0x6


/*
 * Must be implemented in HAL
 */
#define SYSCALL_NONE_HAL            0x10
#define SYSCALL_PING_HAL            0x11
#define SYSCALL_GET_TCB             0x12 // Thread control block. Only effective only systems without fast TCB support
#define SYSCALL_IOPORT_OUT          0x13 // IO ports
#define SYSCALL_IOPORT_IN           0x14 // Only effective on systems with IO ports (e.g. x86)


/*
 * IPC
 */
#define SYSCALL_MSG_HANDLER_REG     0x20
#define SYSCALL_MSG_HANDLER_UNREG   0x21
#define SYSCALL_MSG_SEND            0x22
#define SYSCALL_MSG_REQUEST         0x23
#define SYSCALL_MSG_RECV            0x24
#define SYSCALL_MSG_REPLY           0x25
#define SYSCALL_MSG_RESPOND         0x26


/*
 * Max syscall num
 */
#define MAX_NUM_SYSCALLS            0x30


#endif
