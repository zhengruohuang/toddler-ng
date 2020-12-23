#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/stdlib.h"
#include "system/include/test.h"
#include "libsys/include/syscall.h"


static void test_worker(ulong param)
{
    kprintf("Testing\n");
    //test_syscall();
    //test_thread();
    //test_ipc();
    test_malloc();
    kprintf("Passed all tests!\n");

    syscall_thread_exit_self(0);
}

void _start()
{
    init_kprintf();
    init_ipc();
    init_malloc();
    syscall_thread_create(test_worker, 0);

    ulong seconds = 0;
    while (1) {
        kprintf("System: %lu seconds\n", seconds++);
        syscall_wait_on_timeout(1000);
        //syscall_thread_exit_self(0);
    }

    while (1);
}
