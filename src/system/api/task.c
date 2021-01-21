#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <sys/api.h>

#include "system/include/task.h"


/*
 * Error response
 */
static inline void _err_response(int err)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, err);
    syscall_ipc_respond();
}


/*
 * Create
 */
static ulong vfs_api_task_create(ulong opcode)
{
    _err_response(-1);
    return 0;
}


/*
 * Exit
 */
static ulong vfs_api_exit(ulong opcode)
{
    _err_response(-1);
    return 0;
}


/*
 * Detach
 */
static ulong vfs_api_detach(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    ulong status = msg_get_param(msg, 0);
    pid_t pid = msg->sender.pid;

    // Detach
    int err = -1;
    access_task(pid, t) {
        t->exit_status = status;
        cond_signal(&t->exit);
        err = 0;
    }

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);
    syscall_ipc_respond();

    return 0;
}


/*
 * Wait
 */
static ulong vfs_api_wait(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    pid_t pid = msg_get_param(msg, 0);

    // Wait
    int err = -1;
    unsigned long status = 0;
    access_task(pid, t) {
        cond_wait(&t->exit);
        err = 0;
        status = t->exit_status;
    }

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);           // err
    if (!err) {
        msg_append_param(msg, status);  // status
    }
    syscall_ipc_respond();

    return 0;
}


/*
 * Init
 */
void init_task_api()
{
    register_msg_handler(SYS_API_TASK_CREATE, vfs_api_task_create);
    register_msg_handler(SYS_API_EXIT, vfs_api_exit);
    register_msg_handler(SYS_API_DETACH, vfs_api_detach);
    register_msg_handler(SYS_API_WAIT, vfs_api_wait);
}
