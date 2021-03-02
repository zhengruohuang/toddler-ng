#include <stdio.h>
#include <kth.h>
#include <sys/api.h>


/*
 * Run test program
 */
static inline int run_test(int argc, char **argv, int idx, int total)
{
    kprintf("====== %d / %d ======\n", idx, total);

    pid_t pid = 0;
    int err = sys_api_task_create(argc, argv, NULL, &pid);
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


/*
 * Monitor
 */
static volatile int stop = 0;
static ref_count_t rounds;

static unsigned long monitor_worker(unsigned long param)
{
    ulong seconds = 0;
    ulong last_round = 0;
    ulong max_rounds_per_sec = 0;

    while (!stop) {
        ulong cur_round = ref_count_read(&rounds);
        if (cur_round) {
            ulong rounds_per_sec = cur_round - last_round;
            if (rounds_per_sec > max_rounds_per_sec) {
                max_rounds_per_sec = rounds_per_sec;
            }

            if (rounds_per_sec < max_rounds_per_sec / 2) {
                stop = 1;
                atomic_mb();

                kprintf("Too slow!\n");
            }
        }

        kprintf("Monitor: %lu seconds, %lu rounds per second\n",
                seconds, cur_round - last_round);
        last_round = cur_round;

        syscall_wait_on_timeout(1000);
        seconds++;
    }

    kprintf("Montor: %lu seconds, max rounds per second: %lu\n",
            seconds, max_rounds_per_sec);
    return 0;
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

    int exec_argc = 1;
    char *exec_argv[] = { "/sys/coreimg/hello.elf", NULL };
    for (int i = 0; i < repeat && !stop; i++) {
        run_test(exec_argc, exec_argv, i + 1, repeat);
        ref_count_inc(&rounds);
    }

    stop = 1;
    atomic_mb();

    kth_join(&monitor, NULL);

    return 0;
}
