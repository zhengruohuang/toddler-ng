#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "hal/include/mp.h"


decl_per_cpu(int, halt_cpu_flag);


void halt_all_cpus(int count, ...)
{

}

void init_halt()
{
    int *flag = get_per_cpu(int, halt_cpu_flag);
    *flag = 0;
}

void init_halt_mp()
{
    init_halt();
}
