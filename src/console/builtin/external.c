#include <stdio.h>
#include <sys/api.h>

int exec_external(int argc, char **argv)
{
    if (!argc || !argv || !argv[0]) {
        return -1;
    }

    const char *pathname = argv[0];
    FILE *f = fopen(pathname, "rb");
    if (!f) {
        kprintf("Unknown command or program: %s\n", pathname);
        return -1;
    }
    fclose(f);

    pid_t pid = 0;
    int err = sys_api_task_create(argc, argv, NULL, &pid);

    if (!err) {
        unsigned long status = 0;
        sys_api_task_wait(pid, &status);
        err = (long)status;
    }

    return err;
}
