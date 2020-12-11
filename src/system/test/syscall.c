#include "common/include/inttypes.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"


static void test(const char *name, ulong num, ulong in, ulong ref_out, ulong flag)
{
    ulong r1 = 0, r2 = 0;
    sysenter(num, in, in, flag, &r1, &r2);
    if (r1 != ref_out || r2 != ref_out) {
        kprintf("Bad output %s: r1 = %x, r2 = %lx\n", name, r1, r2);
    }
}

void test_syscall()
{
    kprintf("Testing syscall\n");

    ulong in = 0x70dd1e2;
    ulong ref_out = in + 1;
    test("HAL", SYSCALL_HAL_PING, in, ref_out, 0);
    test("Kernel", SYSCALL_PING, in, ref_out, 0);
    test("Kernel", SYSCALL_PING, in, ref_out, 1);
    test("Kernel", SYSCALL_PING, in, ref_out, 2);

    kprintf("Passed syscall test!\n");
}
