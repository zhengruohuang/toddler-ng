#include <stdio.h>
#include <sys/api.h>

static int test_hello(int idx, int total)
{
    int exec_argc = 1;
    char *exec_argv[] = { "/sys/coreimg/hello.elf", NULL };

    kprintf("====== %d / %d ======\n", idx, total);

    pid_t pid = 0;
    int err = sys_api_task_create(exec_argc, exec_argv, NULL, &pid);
    kprintf("New task created: %d, pid: %lx\n", err, pid);

    if (!err) {
        unsigned long status = 0;
        sys_api_task_wait(pid, &status);
        kprintf("Task %lx exited with code %ld\n", pid, status);
        err = (long)status;
    }

    kprintf("====== %d / %d ======\n", idx, total);

    return err;
}

int exec_test(int argc, char **argv)
{
    int repeat = 0;
    if (argc >= 2 && argv[1]) {
        for (char *s = argv[1]; *s; s++) {
            char ch = *s;
            if (ch < '0' || ch > '9') {
                break;
            }
            repeat *= 10;
            repeat += ch - '0';
        }
    }

    if (!repeat) {
        repeat = 1;
    }

    for (int i = 0; i < repeat; i++) {
        test_hello(i + 1, repeat);
    }

    return 0;
}
