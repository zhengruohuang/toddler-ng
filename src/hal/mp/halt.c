#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "common/include/atomic.h"
#include "hal/include/mp.h"
#include "hal/include/int.h"
#include "hal/include/hal.h"


static volatile int cpu_halted = 0;

static inline void _halt()
{
    disable_local_int();

    struct hal_arch_funcs *funcs = get_hal_arch_funcs();
    funcs->halt();

    while (1);
}

void halt_all_cpus()
{
    cpu_halted = 1;
    atomic_mb();

    _halt();
    while (1);
}

void handle_halt()
{
    if (cpu_halted) {
        _halt();
        while (1);
    }
}
