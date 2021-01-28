#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>

#include "common/include/inttypes.h"


/*
 * Handlers
 */
#define DEFAULT_NUM_HANDLERS 256

static rwlock_t ipc_handler_rwlock = RWLOCK_INITIALIZER;

static ulong num_handlers = 0;
static msg_handler_t *handlers = NULL; //[255] = { 0 };
static msg_handler_t global_handler = NULL;

static inline void init_handlers(ulong count)
{
    if (handlers) {
        handlers = realloc(handlers, sizeof(msg_handler_t) * count);
    } else {
        handlers = calloc(count, sizeof(msg_handler_t));
    }
    num_handlers = count;
}

static inline void update_handler(ulong port, msg_handler_t handler)
{
    if (!num_handlers) {
        init_handlers(DEFAULT_NUM_HANDLERS);
    }

    if (port == GLOBAL_MSG_PORT) {
        global_handler = handler;
    } else {
        if (port >= num_handlers) {
            init_handlers(port + 1);
        }
        handlers[port] = handler;
    }
}

static inline msg_handler_t find_handler(ulong port)
{
    if (!num_handlers) {
        return NULL;
    }

    if (port == GLOBAL_MSG_PORT) {
        return global_handler;
    } else if (port < num_handlers) {
        return handlers[port];
    }

    return NULL;
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
    rwlock_rlock(&ipc_handler_rwlock);

    msg_handler_t handler = find_handler(port);
    if (!handler) {
        rwlock_runlock(&ipc_handler_rwlock);
        return NULL;
    }

    return handler;
}

static void release_handler(ulong port)
{
    rwlock_runlock(&ipc_handler_rwlock);
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
    syscall_ipc_handler(dispatch);
}


/*
 * Register
 */
int register_msg_handler(ulong port, msg_handler_t handler)
{
    if (port > GLOBAL_MSG_PORT) {
        return -1;
    }

    rwlock_exclusive_w(&ipc_handler_rwlock) {
        update_handler(port, handler);
    }

    return 0;
}

int cancel_msg_handler(ulong port)
{
    return register_msg_handler(port, NULL);
}
