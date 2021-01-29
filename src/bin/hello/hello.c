#include <stdio.h>
#include <kth.h>
#include <sys.h>


/*
 * Clock
 */
#define CLOCK_PERIOD_SECONDS 5

static unsigned long clock_worker(unsigned long param)
{
    ulong seconds = 0;
    while (1) {
        kprintf("Hello: %lu seconds\n", seconds);

        syscall_wait_on_timeout(CLOCK_PERIOD_SECONDS * 1000);
        seconds += CLOCK_PERIOD_SECONDS;
    }

    return 0;
}

static void clock()
{
    kth_t thread;
    kth_create(&thread, clock_worker, 0);
}


/*
 * Main
 */
int main(int argc, char **argv)
{
    kprintf("Hello!\n");
    clock();
}
