#ifndef __COMMON_INCLUDE_PROC__
#define __COMMON_INCLUDE_PROC__


#include "common/include/inttypes.h"
#include "common/include/ipc.h"


/*
 * Thread entry
 */
typedef void (*thread_entry_t)(ulong param);


/*
 * Thread info block
 */
typedef struct thread_info_block {
    struct thread_info_block *self;

    ulong pid;
    ulong tid;

    msg_t *msg;

    void *tls;
    ulong tls_size;

    // For user-level libs
    void *kthread;
} thread_info_block_t;


/*
 * Thread wait type
 */
enum thread_wait_type {
    WAIT_ON_TIMEOUT,
    WAIT_ON_FUTEX,
    WAIT_ON_MSG_REPLY,
    WAIT_ON_THREAD,
    WAIT_ON_MAIN_THREAD,
};


/*
 * Futex
 */
typedef struct futex {
    union {
        ulong value;

        struct {
            ulong valid         : 1;
            ulong kernel        : 1;
            ulong locked        : sizeof(ulong) - 2;
        };
    };
} futex_t;

#define FUTEX_INITIALIZER   { .valid = 1 }


/*
 * VM attributes
 */
enum vm_attri {
    VM_ATTRI_READ           = 0x1,
    VM_ATTRI_WRITE          = 0x2,
    VM_ATTRI_EXEC           = 0x4,
    VM_ATTRI_CACHED         = 0x8,
    VM_ATTRI_DIRECT_MAPPED  = 0x10,

    VM_ATTRI_CODE           = VM_ATTRI_CACHED | VM_ATTRI_READ | VM_ATTRI_EXEC,
    VM_ATTRI_CODE_JIT       = VM_ATTRI_CACHED | VM_ATTRI_READ | VM_ATTRI_WRITE | VM_ATTRI_EXEC,
    VM_ATTRI_DATA           = VM_ATTRI_CACHED | VM_ATTRI_READ | VM_ATTRI_WRITE,
    VM_ATTRI_DATA_RO        = VM_ATTRI_CACHED | VM_ATTRI_READ,
    VM_ATTRI_DEV            = VM_ATTRI_READ | VM_ATTRI_WRITE,
    VM_ATTRI_DEV_RO         = VM_ATTRI_READ,
};


#endif
