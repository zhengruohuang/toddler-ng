#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>

#include "system/include/task.h"
#include "system/include/vfs.h"
#include "system/include/test.h"
#include "system/include/startup.h"


/*
 * Run
 */
static ulong start_system_worker(ulong param)
{
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
    init_task();

    init_vfs();
    init_vfs_api();

    init_rootfs();
    init_coreimgfs();
}


/*
 * Main
 */
static void clock()
{
    ulong seconds = 0;
    while (1) {
        kprintf("System: %lu seconds\n", seconds++);
        syscall_wait_on_timeout(1000);
    }
}

int main(int argc, char **argv)
{
    init_system();
    start_system();

    clock();

    while (1);
    return 0;
}
