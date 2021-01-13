#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>
#include <hal.h>

#include "device/include/devtreefs.h"


/*
 * Test
 */
extern void test_devtree();

__unused_func static void test_device()
{
    test_devtree();
}


/*
 * Init
 */
static void init_device()
{
    init_devtreefs();
}


/*
 * Driver
 */
static void start_drivers()
{
    extern void init_pl011_driver();
    init_pl011_driver();
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
    //test_device();

    kprintf("Device, argc: %d, argv[0]: %s\n", argc, argv[0]);
    start_drivers();

    clock();

    while (1);
    return 0;
}
