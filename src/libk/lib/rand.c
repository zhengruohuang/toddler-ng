#include "common/include/inttypes.h"


static unsigned int rand_state = 0;


int rand()
{
    rand_state = (rand_state * 1103515245u) + 12345u;
    return rand_state & 0x7fffffff;
}

int rand_r(unsigned int *seedp)
{
    if (!seedp) {
        return 0;
    }

    unsigned int r = *seedp;
    r = (r * 1103515245u) + 12345u;
    *seedp = r;
    return r & 0x7fffffff;
}

void srand(unsigned int seed)
{
    rand_state = seed;
}
