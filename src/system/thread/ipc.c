#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"
#include "libsys/include/ipc.h"
#include "libk/include/string.h"
#include "libk/include/debug.h"


/*
 * Handlers
 */
static fast_rwlock_t global_rwlock = FAST_RWLOCK_INITIALIZER;

// TODO: use hashtable
static msg_handler_t handlers[255] = { 0 };
static msg_handler_t global_handler = NULL;

static inline msg_handler_t find_handler(ulong port)
{
    if (port == GLOBAL_MSG_PORT) {
        return global_handler;
    } else {
        return handlers[port];
    }
}

static inline void update_handler(ulong port, msg_handler_t handler)
{
    if (port == GLOBAL_MSG_PORT) {
        global_handler = handler;
    } else {
        handlers[port] = handler;
    }
}


/*
 * Dispatcher
 */
static inline ulong get_port(ulong opcode)
{
    return opcode & MSG_OPCODE_MASK;
}

static msg_handler_t acquire_handler(ulong port)
{
    fast_rwlock_rdlock(&global_rwlock);

    msg_handler_t handler = find_handler(port);
    if (!handler) {
        fast_rwlock_rdunlock(&global_rwlock);
        return NULL;
    }

    return handler;
}

static void release_handler(ulong port)
{
    fast_rwlock_rdunlock(&global_rwlock);
}

static void dispatch(ulong opcode)
{
    //kprintf("IPC dispatch, opcode: %lu\n", opcode);

    ulong port = get_port(opcode);
    ulong ret = 0;

    msg_handler_t handler = acquire_handler(port);
    if (handler) {
        ret = handler(opcode);
        release_handler(port);
    }

    //syscall_ipc_respond();
    syscall_thread_exit_self(ret);
}


/*
 * Init
 */
void init_ipc()
{
    fast_rwlock_init(&global_rwlock);
    syscall_ipc_handler(dispatch);
}


/*
 * Register
 */
void register_msg_handler(ulong port, msg_handler_t handler)
{
    fast_rwlock_wrlock(&global_rwlock);
    update_handler(port, handler);
    fast_rwlock_wrunlock(&global_rwlock);
}

void cancel_msg_handler(ulong port)
{
    register_msg_handler(port, NULL);
}
