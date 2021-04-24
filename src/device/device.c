#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>
#include <hal.h>

#include "common/include/abi.h"
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
    //test_dev();
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
extern void init_pl011_driver();
extern void init_ns16550_driver();
extern void init_or1k_ns16550_driver();

extern void init_dev_zero();
extern void init_dev_null();
extern void init_dev_full();
extern void init_dev_random();

static void start_drivers()
{
#if defined(ARCH_ARMV7)
    init_pl011_driver();
#elif (defined(ARCH_MIPS) || defined(ARCH_OPENRISC) || defined(ARCH_RISCV) || defined(ARCH_X86))
    init_ns16550_driver();
#endif

    init_dev_zero();
    init_dev_null();
    init_dev_null();
    init_dev_random();
}


/*
 * Clock
 */
#define CLOCK_PERIOD_SECONDS 10

static unsigned long clock_worker(unsigned long param)
{
    ulong seconds = 0;
    while (1) {
        kprintf("Device: %lu seconds\n", seconds);

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
    init_device();
    start_drivers();
    test_device();

    kprintf("Device, argc: %d, argv[0]: %s\n", argc, argv[0]);
    clock();

    // Detach
    sys_api_task_detach(0);

    // Daemon
    syscall_thread_exit_self(0);

    panic("Should not reach here!\n");
    while (1);
    return -1;
}
