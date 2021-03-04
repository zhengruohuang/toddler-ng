#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys/api.h>


/*
 * Monitor
 */
static volatile int stop = 0;

static unsigned long monitor_worker(unsigned long param)
{
    ulong seconds = 0;

    while (!stop) {
        kprintf("Monitor: %lu seconds\n", seconds);

        syscall_wait_on_timeout(1000);
        seconds++;
    }

    kprintf("Montor: %lu seconds, stopped\n", seconds);
    return 0;
}


/*
 * Timer
 */
struct timer_param {
    union {
        ulong value;

        struct {
            ulong idx       : 8;
            ulong rounds    : 8;
            ulong step      : sizeof(ulong) * 8 - 16;
        };
    };
};

static unsigned long timer_worker(unsigned long param)
{
    ulong ms = 0;
    struct timer_param tp = { .value = param };

    for (int i = 0; i < tp.rounds; i++) {
        kprintf("Timer #%d: %lu ms\n", tp.idx, ms);

        syscall_wait_on_timeout(tp.step);
        ms += tp.step;
    }

    kprintf("Timer #%d done: %lu ms\n", tp.idx, ms);
    return 0;
}


int exec_timer(int argc, char **argv)
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

    stop = 0;
    atomic_mb();

    kth_t monitor;
    kth_create(&monitor, monitor_worker, 0);

    kth_t *timer_threads = malloc(sizeof(kth_t) * 10);
    for (int i = 0; i < 10; i++) {
        struct timer_param param;
        param.idx = i;
        param.rounds = 20;
        param.step = 170 + 135 * i;

        kth_create(&timer_threads[i], timer_worker, param.value);
    }

    for (int i = 0; i < 10; i++) {
        kth_join(&timer_threads[i], NULL);
    }

    free(timer_threads);

    stop = 1;
    atomic_mb();

    kth_join(&monitor, NULL);
    return 0;
}
