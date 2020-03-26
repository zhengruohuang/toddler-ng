#include "common/include/inttypes.h"
#include "libk/include/kprintf.h"


static periph_putchar_t putchar_func = NULL;


void init_libk_putchar(periph_putchar_t func)
{
    putchar_func = func;
}

int __kputchar(int ch)
{
    if (putchar_func) {
        putchar_func(ch);
        return 1;
    }

    return 0;
}
