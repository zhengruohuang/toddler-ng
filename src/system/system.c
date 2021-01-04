#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>

#include "system/include/vfs.h"
#include "system/include/test.h"


/*
 * Test
 */
static void test_worker(ulong param)
{
    kprintf("Testing\n");
    //test_syscall();
    //test_thread();
    //test_ipc();
    //test_malloc();
    test_vfs();
    kprintf("Passed all tests!\n");

    syscall_thread_exit_self(0);
}

static void test_system()
{
    syscall_thread_create(test_worker, 0);
}


/*
 * Init
 */
static void init_system()
{
    init_ipc();
    init_vfs();
    init_vfs_api();
    init_rootfs();
    init_coreimgfs();
}

int main(int argc, char **argv)
{
    init_system();
    test_system();

    ulong seconds = 0;
    while (1) {
        kprintf("System: %lu seconds\n", seconds++);
        syscall_wait_on_timeout(1000);
    }

    while (1);
    return 0;
}
