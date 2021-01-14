#include <stdio.h>
#include <stdlib.h>
#include <kth.h>
#include <sys.h>
#include <hal.h>

#include "device/include/devtreefs.h"
#include "device/include/devfs.h"


/*
 * Test
 */
extern void test_devtree();
extern void test_dev();

__unused_func static void test_device()
{
    //test_devtree();
    test_dev();
}


/*
 * Init
 */
static void init_device()
{
    init_devtreefs();
    init_devfs();
}


/*
 * Driver
 */
static void start_drivers()
{
    extern void init_pl011_driver();
    init_pl011_driver();

    extern void init_dev_zero();
    init_dev_zero();

    extern void init_dev_null();
    init_dev_null();
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
    start_drivers();
    test_device();

    kprintf("Device, argc: %d, argv[0]: %s\n", argc, argv[0]);


    clock();

    while (1);
    return 0;
}
