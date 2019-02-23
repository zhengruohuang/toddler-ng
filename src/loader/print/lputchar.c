#include "common/include/inttypes.h"
#include "loader/include/lprintf.h"


extern int arch_debug_putchar(int ch);

static periph_putchar_t putchar_func = NULL;


void init_putchar(periph_putchar_t func)
{
    putchar_func = func;
}

int lputchar(int ch)
{
    if (putchar_func) {
        putchar_func(ch);
        return 1;
    }

    arch_debug_putchar(ch);
    return 1;
}
