#include "common/include/inttypes.h"
#include "libk/include/kprintf.h"
#include "libsys/include/syscall.h"
#include "system/include/thread.h"


/*
 * putchar
 */
#define PUTS_BUF_SIZE   256

static int puts_buf_idx = 0;
static char puts_buf[PUTS_BUF_SIZE + 1];

static int kputchar(int ch)
{
    puts_buf[puts_buf_idx++] = ch;
    if (ch == '\n' || puts_buf_idx == PUTS_BUF_SIZE) {
        puts_buf[puts_buf_idx] = '\0';
        syscall_puts(puts_buf, puts_buf_idx + 1);
        puts_buf_idx = 0;
    }

    return 1;
}


/*
 * kprintf
 */
static mutex_t kprintf_mutex = MUTEX_INITIALIZER;

int kprintf(const char *fmt, ...)
{
    int ret = 0;
    va_list va;
    va_start(va, fmt);

    mutex_lock(&kprintf_mutex);
    puts_buf_idx = 0;
    ret = __vkprintf(fmt, va);
    mutex_unlock(&kprintf_mutex);

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
    mutex_init(&kprintf_mutex);
}
