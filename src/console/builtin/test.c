#include <stdio.h>
#include <kth.h>
#include <sys/api.h>

static volatile int stop = 0;
static ref_count_t rounds;

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

static unsigned long monitor_worker(unsigned long param)
{
    ulong last_round = 0;
    ulong seconds = 0;
    while (!stop) {
        ulong cur_round = ref_count_read(&rounds);
        if (cur_round && cur_round - last_round < 10) {
            stop = 1;
            kprintf("Too slow!\n");
        }

        kprintf("Monitor: %lu seconds, %lu rounds per second\n", seconds, cur_round - last_round);
        last_round = cur_round;

        syscall_wait_on_timeout(1000);
        seconds++;
    }

    return 0;
}

static void monitor()
{
    kth_t thread;
    kth_create(&thread, monitor_worker, 0);
    kth_join(&thread, NULL);
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

    kth_t monitor;

    stop = 0;
    ref_count_init(&rounds, 0);

    atomic_mb();
    kth_create(&monitor, monitor_worker, 0);

    for (int i = 0; i < repeat && !stop; i++) {
        test_hello(i + 1, repeat);
        ref_count_inc(&rounds);
    }

    stop = 1;
    kth_join(&monitor, NULL);

    return 0;
}
