// #include "common/include/inttypes.h"
#include "hal/include/kprintf.h"
// #include "hal/include/mp.h"


static int num_cpus = 0;


int get_num_cpus()
{
    return num_cpus;
}

void init_topo()
{
    kprintf("Detecting processor topology\n");

    num_cpus = 1;

    kprintf("\tNumber of logical CPUs: %d\n", num_cpus);
}
