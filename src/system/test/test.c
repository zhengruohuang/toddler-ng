#include <stdio.h>
#include "system/include/test.h"


void run_tests()
{
    kprintf("Testing\n");
    //test_syscall();
    //test_thread();
    //test_ipc();
    //test_malloc();
    test_vfs();
    kprintf("Passed all tests!\n");
}
