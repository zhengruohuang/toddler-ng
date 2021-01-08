#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>

#include "device/include/devtreefs.h"


/*
 * Init
 */
static void init_device()
{
    init_devtreefs();
}


/*
 * Main
 */
static void clock()
{
    ulong seconds = 0;
    while (1) {
        kprintf("Device: %lu seconds\n", seconds++);
        syscall_wait_on_timeout(1000);
    }
}

int main(int argc, char **argv)
{
    init_device();

    clock();

    while (1);
    return 0;
}
