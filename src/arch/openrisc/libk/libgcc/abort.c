#include "common/include/compiler.h"

extern void __stop();

void abort()
{
    __stop();
    while (1);
}
