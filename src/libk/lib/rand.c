#include "common/include/inttypes.h"


static u32 rand_state = 0;


int rand()
{
    rand_state = (rand_state * 1103515245u) + 12345u;
    return rand_state & 0x7fffffff;
}

int rand_r(u32 *seedp)
{
    if (!seedp) {
        return 0;
    }

    u32 r = *seedp;
    r = (r * 1103515245u) + 12345u;
    *seedp = r;
    return r & 0x7fffffff;
}

void srand(u32 seed)
{
    rand_state = seed;
}
