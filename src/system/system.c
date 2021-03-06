#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>

#include "system/include/task.h"
#include "system/include/vfs.h"
#include "system/include/api.h"
#include "system/include/test.h"


/*
 * Run
 */
static ulong start_system_worker(ulong param)
{
    kprintf("starting up");
    // Test
    run_tests();

    // Startup
    startup();

    return 0;
}

static void start_system()
{
    kth_t thread;
    kth_create(&thread, start_system_worker, 0);
}


/*
 * Init
 */
static void init_system()
{
    // System components
    init_task();
    init_vfs();

    // API
    init_vfs_api();
    init_task_api();

    // File systems
    init_rootfs();
    init_coreimgfs();
}


/*
 * Clock
 */
#define CLOCK_PERIOD_SECONDS 10

static void clock()
{
    ulong seconds = 0;
    while (1) {
        kprintf("System: %lu seconds\n", seconds);

        syscall_wait_on_timeout(CLOCK_PERIOD_SECONDS * 1000);
        seconds += CLOCK_PERIOD_SECONDS;
    }
}


/*
 * Main
 */
int main(int argc, char **argv)
{
    kprintf("System process started!\n");

    init_system();
    start_system();

    clock();

    while (1);
    return 0;
}
