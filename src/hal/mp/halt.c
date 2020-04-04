#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "hal/include/mp.h"
#include "hal/include/int.h"
#include "hal/include/hal.h"


static volatile int cpu_halted = 0;


void halt_all_cpus(int count, ...)
{
    disable_local_int();

    // FIXME: atomic
    cpu_halted = 1;

    va_list args;
    va_start(args, count);

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    funcs->halt(count, args);

    va_end(args);
    while (1);
}

void handle_halt()
{
    // FIXME: atomic
    if (cpu_halted) {
        disable_local_int();

        va_list args;
        struct hal_arch_funcs *funcs = get_hal_arch_funcs();
        funcs->halt(0, args);

        while (1);
    }
}
