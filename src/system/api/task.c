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
    // Request
    msg_t *msg = get_msg();
    pid_t ppid = msg->sender.pid;

    int task_type = msg_get_int(msg, 0);
    const char *task_work_dir = msg_get_data(msg, 1, NULL);
    const char *task_stdin = msg_get_data(msg, 2, NULL);
    const char *task_stdout = msg_get_data(msg, 3, NULL);
    const char *task_stderr = msg_get_data(msg, 4, NULL);

    int task_argc = msg_get_int(msg, 5);
    char **task_argv = calloc(task_argc + 1, sizeof(char *));
    for (int i = 0; i < task_argc; i++) {
        task_argv[i] = msg_get_data(msg, 6 + i, NULL);
    }
    task_argv[task_argc] = NULL;

    // Create task
    pid_t pid = task_create(ppid, task_type, task_argc, task_argv, NULL);
    free(task_argv);

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, pid ? 0 : -1);
    msg_append_param(msg, pid);
    syscall_ipc_respond();

    return 0;
}


/*
 * Exit
 */
static ulong task_api_exit(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    ulong status = msg_get_param(msg, 0);
    pid_t pid = msg->sender.pid;

    // Exit
    int err = task_exit(pid, status);

    // Response
    msg = get_empty_msg();
    msg_append_int(msg, err);
    syscall_ipc_respond();

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
    int err = task_detach(pid, status);

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
 * System notifications
 */
static ulong task_notify_process_stopped(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    pid_t pid = msg_get_param(msg, 0);

    //kprintf("Process stopped: %lx\n", pid);
    task_cleanup(pid);

    return 0;
}

static ulong task_notify_process_crashed(ulong opcode)
{
    // Request
    msg_t *msg = get_msg();
    pid_t pid = msg_get_param(msg, 0);

    kprintf("Process crashed: %lu\n", pid);

    return 0;
}


/*
 * Init
 */
void init_task_api()
{
    register_msg_handler(SYS_NOTIF_PROCESS_STOPPED, task_notify_process_stopped);
    register_msg_handler(SYS_NOTIF_PROCESS_CRASHED, task_notify_process_crashed);

    register_msg_handler(SYS_API_TASK_CREATE, task_api_task_create);
    register_msg_handler(SYS_API_EXIT, task_api_exit);
    register_msg_handler(SYS_API_DETACH, task_api_detach);
    register_msg_handler(SYS_API_WAIT, task_api_wait);
    register_msg_handler(SYS_API_TASK_SET, task_api_set_attri);
    register_msg_handler(SYS_API_TASK_GET, task_api_get_attri);
}
