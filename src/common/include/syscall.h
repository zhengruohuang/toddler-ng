#ifndef __COMMON_INCLUDE_SYSCALL__
#define __COMMON_INCLUDE_SYSCALL__


#include "common/include/inttypes.h"


enum syscall_nums {
    // Must be implemented in HAL
    SYSCALL_HAL_NONE,
    SYSCALL_HAL_PING,
    SYSCALL_HAL_GET_TIB,    // Thread info block. Only used on systems without fast TIB support
    SYSCALL_HAL_IOPORT,     // IO ports. Only used on systems with IO ports (e.g. x86)

    // System
    SYSCALL_NONE,
    SYSCALL_PING,
    SYSCALL_PUTS,

    // Illegal
    SYSCALL_ILLEGAL,

    // Interrupt
    SYSCALL_INTERRUPT,
    SYSCALL_INT_ALLOC,
    SYSCALL_INT_HANDLER,
    SYSCALL_INT_EOI,

    // Fault
    SYSCALL_FAULT_PAGE,
    SYSCALL_FAULT_UNALIGN,
    SYSCALL_FAULT_INSTR,

    // Process
    SYSCALL_PROCESS_CREATE,
    SYSCALL_PROCESS_EXIT,
    SYSCALL_PROCESS_RECYCLE,

    // VM
    SYSCALL_VM_ALLOC,
    SYSCALL_VM_MAP,
    SYSCALL_VM_MAP_CROSS,
    SYSCALL_VM_FREE,

    // Thread
    SYSCALL_THREAD_CREATE,
    SYSCALL_THREAD_CREATE_CROSS,
    SYSCALL_THREAD_YIELD,
    SYSCALL_THREAD_EXIT,

    // Event
    SYSCALL_EVENT_ALLOC,
    SYSCALL_EVENT_WAIT,
    SYSCALL_EVENT_WAKE,

    // IPC
    SYSCALL_IPC_HANDLER,
    SYSCALL_IPC_REQUEST,    // Send the msg and block sending thread until a response is received
    SYSCALL_IPC_RESPOND,    // Send a response to the sender
    SYSCALL_IPC_RECEIVE,    // Receive a serial msg

    // Stats
    SYSCALL_STATS_KERNEL,

    // Num syscalls
    NUM_SYSCALLS
};


#endif
