#include <stdio.h>
#include <sys/api.h>

int exec_test(int argc, char **argv)
{
    int exec_argc = 1;
    char *exec_argv[] = { "/sys/coreimg/hello.elf", NULL };

    pid_t pid = 0;
    int err = sys_api_task_create(exec_argc, exec_argv, NULL, &pid);
    kprintf("New task created: %d, pid: %lu\n", err, pid);

    if (!err) {
        unsigned long status = 0;
        sys_api_task_wait(pid, &status);
        kprintf("Task %lu exited with code %ld\n", pid, status);
        err = (long)status;
    }

    return err;
}
