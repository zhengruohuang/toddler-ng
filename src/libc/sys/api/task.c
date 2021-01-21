#include <stdio.h>
#include <sys.h>
#include <sys/api.h>


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
