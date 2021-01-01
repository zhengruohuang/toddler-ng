#include <futex.h>
#include <sys.h>
#include "libk/include/kprintf.h"


/*
 * putchar
 */
#define PUTS_BUF_SIZE   256

static int puts_buf_idx = 0;
static char puts_buf[PUTS_BUF_SIZE + 1];

static int flush_puts_buf()
{
    int ret = puts_buf_idx;

    puts_buf[puts_buf_idx] = '\0';
    syscall_puts(puts_buf, puts_buf_idx + 1);
    puts_buf_idx = 0;

    return ret;
}

static int kputchar(int ch)
{
    puts_buf[puts_buf_idx++] = ch;
    if (ch == '\n' || puts_buf_idx == PUTS_BUF_SIZE) {
        flush_puts_buf();
    }

    return 1;
}


/*
 * kprintf
 */
static futex_t kprintf_futex = FUTEX_INITIALIZER;

int kprintf(const char *fmt, ...)
{
    int ret = 0;
    va_list va;
    va_start(va, fmt);

    futex_exclusive(&kprintf_futex) {
        puts_buf_idx = 0;
        ret = __vkprintf(fmt, va);
        ret += flush_puts_buf();
    }

    va_end(va);
    return ret;
}

int kprintf_unlocked(const char *fmt, ...)
{
    int ret = 0;
    puts_buf_idx = 0;

    va_list va;
    va_start(va, fmt);
    ret = __vkprintf(fmt, va);
    va_end(va);

    return ret;
}


/*
 * Init
 */
void init_kprintf()
{
    init_libk_putchar(kputchar);
}
