#include <stdio.h>
#include <string.h>
#include <sys.h>
#include <sys/api.h>


/*
 * Create and exit
 */
int sys_api_task_create(int type, int argc, char **argv, const char *stdio[3])
{
    return -1;
}

int sys_api_task_exit(unsigned long status)
{
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, status);
    syscall_ipc_popup_request(0, SYS_API_EXIT);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}


/*
 * Detach and wait
 */
int sys_api_task_detach(unsigned long status)
{
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, status);
    syscall_ipc_popup_request(0, SYS_API_DETACH);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}

int sys_api_task_wait(pid_t pid, unsigned long *status)
{
    msg_t *msg = get_empty_msg();
    msg_append_param(msg, pid);
    syscall_ipc_popup_request(0, SYS_API_WAIT);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    if (err) {
        return err;
    }

    if (status) {
        *status = msg_get_param(msg, 1);
    }

    return 0;
}


/*
 * Attri
 */
int sys_api_task_set_work_dir(const char *pathname)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, TASK_ATTRI_WORK_DIR);
    msg_append_str(msg, pathname, 0);
    syscall_ipc_popup_request(0, SYS_API_TASK_SET);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    return err;
}

int sys_api_task_get_work_dir(char *buf, size_t size)
{
    msg_t *msg = get_empty_msg();
    msg_append_int(msg, TASK_ATTRI_WORK_DIR);
    syscall_ipc_popup_request(0, SYS_API_TASK_GET);

    msg = get_response_msg();
    int err = msg_get_int(msg, 0);
    const char *cwd = msg_get_data(msg, 1, NULL);
    if (buf && size) {
        strncpy(buf, cwd, size);
        buf[size - 1] = '\0';
    }

    return err;
}
