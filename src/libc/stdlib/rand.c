#include <stdlib.h>

int random()
{
    return rand();
}

int random_r(unsigned int *seed)
{
    return rand_r(seed);
}

void srandom(unsigned int seed)
{
    srand(seed);
}
