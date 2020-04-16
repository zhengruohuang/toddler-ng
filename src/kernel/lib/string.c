#include "common/include/inttypes.h"
#include "kernel/include/mem.h"
#include "kernel/include/lib.h"


char *strdup(const char *s)
{
    size_t size = strlen(s) + 1;
    char *dest = malloc(size);

    strcpy(dest, s);
    return dest;
}
