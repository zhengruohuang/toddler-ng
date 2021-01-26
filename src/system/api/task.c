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
static ulong task_api_task_create(ulong opcode)
{
    _err_response(-1);
    return 0;
}


/*
 * Exit
 */
static ulong task_api_exit(ulong opcode)
{
    _err_response(-1);
    return 0;
}


/*
 * Detach
 */
static ulong task_api_detach(ulong opcode)
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
static ulong task_api_wait(ulong opcode)
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
 * Set attri
 */
static ulong task_api_set_attri(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int attri = msg_get_int(msg, 0);    // attri
    pid_t pid = msg->sender.pid;

    // Set
    int err = -1;
    switch (attri) {
    case TASK_ATTRI_NONE:
        err = 0;
        break;
    case TASK_ATTRI_WORK_DIR: {
        const char *pathname = msg_get_data(msg, 1, NULL);  // pathname
        err = task_set_work_dir(pid, pathname);
        break;
    }
    default:
        break;
    }

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);           // err
    syscall_ipc_respond();

    return 0;
}


/*
 * Get attri
 */
static ulong task_api_get_attri(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    int attri = msg_get_int(msg, 0);    // attri
    pid_t pid = msg->sender.pid;

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, 0);             // err

    // Get
    int err = -1;
    switch (attri) {
    case TASK_ATTRI_NONE:
        err = 0;
        break;
    case TASK_ATTRI_WORK_DIR: {
        size_t max_size = msg_remain_data_size(msg) - 32ul;
        char *pathname = msg_append_str(msg, NULL, max_size);   // pathname
        err = task_get_work_dir(pid, pathname, max_size);
        break;
    }
    default:
        break;
    }

    // Done
    msg_set_int(msg, 0, err);            // err
    syscall_ipc_respond();

    return 0;
}


/*
 * Init
 */
void init_task_api()
{
    register_msg_handler(SYS_API_TASK_CREATE, task_api_task_create);
    register_msg_handler(SYS_API_EXIT, task_api_exit);
    register_msg_handler(SYS_API_DETACH, task_api_detach);
    register_msg_handler(SYS_API_WAIT, task_api_wait);
    register_msg_handler(SYS_API_TASK_SET, task_api_set_attri);
    register_msg_handler(SYS_API_TASK_GET, task_api_get_attri);
}
