#include <stdio.h>
#include <stdarg.h>
#include <internal/printf.h>


/*
 * Core
 */
int printf_core(const char *fmt, struct printf_closure *closure, va_list args)
{
    char *s = (char *)fmt;
    int len = 0;
    struct printf_token token;

    while (*s) {
        switch (*s) {
        case '%':
            s++;
            _printf_token(&s, &token);

            switch (token.type) {
            case PRINTF_TOKEN_NONE:
                s--;
                token.va_ch = *s;
                len += _printf_char(closure, &token);
                break;
            case PRINTF_TOKEN_PERCENT:
                token.va_ch = '%';
                len += _printf_char(closure, &token);
                break;
            case PRINTF_TOKEN_CHAR:
                token.va_ch = va_arg(args, int);
                len += _printf_char(closure, &token);
                break;
            case PRINTF_TOKEN_U32:
                token.va_u32 = va_arg(args, u32);
                len += _printf_u32(closure, &token);
                break;
            case PRINTF_TOKEN_U64:
                token.va_u64 = va_arg(args, u64);
                len += _printf_u64(closure, &token);
                break;
            case PRINTF_TOKEN_ULONG:
                token.va_ulong = va_arg(args, ulong);
                len += _printf_ulong(closure, &token);
                break;
            case PRINTF_TOKEN_STR:
                token.va_str = va_arg(args, char *);
                len += _printf_str(closure, &token);
                break;
            default:
                break;
            }
            break;
        default:
            token.va_ch = *s;
            len += _printf_char(closure, &token);
            break;
        }

        s++;
    }

    closure->putchar(0, closure);
    return len;
}


/*
 * Closure
 */
#define PRINTF_FPUTCHAR_BUF_SIZE 256

static void _printf_putchar_file_flush(struct printf_closure *closure)
{
    fwrite(closure->buf, 1, closure->buf_idx, closure->file);
    closure->buf_idx = 0;
}

int _printf_putchar_file(int ch, struct printf_closure *closure)
{
    if (!ch) {
        _printf_putchar_file_flush(closure);
        return 0;
    }

    closure->buf[closure->buf_idx++] = (char)ch;
    if (closure->buf_idx >= closure->buf_size) {
        _printf_putchar_file_flush(closure);
    }

    return 1;
}

int _printf_putchar_buf(int ch, struct printf_closure *closure)
{
    if (!ch) {
        return 0;
    }

    if (closure->buf_size >= 0 && closure->buf_idx >= closure->buf_size) {
        return 0;
    }

    closure->buf[closure->buf_idx++] = (char)ch;
    return 1;
}


/*
 * sprintf
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
    struct printf_closure closure;
    closure.putchar = _printf_putchar_buf;
    closure.buf = buf;
    closure.buf_size = -1;
    closure.buf_idx = 0;

    return printf_core(fmt, &closure, args);
}

int sprintf(char *buf, const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vsprintf(buf, fmt, va);

    va_end(va);
    return count;
}


/*
 * snprintf
 */
int vsnprintf(char *buf, size_t bufsz, const char *fmt, va_list args)
{
    struct printf_closure closure;
    closure.putchar = _printf_putchar_buf;
    closure.buf = buf;
    closure.buf_size = bufsz;
    closure.buf_idx = 0;

    return printf_core(fmt, &closure, args);
}

int snprintf(char *buf, size_t bufsz, const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vsnprintf(buf, bufsz, fmt, va);

    va_end(va);
    return count;
}


/*
 * fprintf
 */
int vfprintf(FILE *f, const char *fmt, va_list args)
{
    char buf[PRINTF_FPUTCHAR_BUF_SIZE];
    struct printf_closure closure;
    closure.putchar = _printf_putchar_file;
    closure.file = f;
    closure.buf = buf;
    closure.buf_size = PRINTF_FPUTCHAR_BUF_SIZE;
    closure.buf_idx = 0;

    return printf_core(fmt, &closure, args);
}

int fprintf(FILE *f, const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vfprintf(f, fmt, va);

    va_end(va);
    return count;
}


/*
 * printf
 */
int vprintf(const char *fmt, va_list args)
{
    char buf[PRINTF_FPUTCHAR_BUF_SIZE];
    struct printf_closure closure;
    closure.putchar = _printf_putchar_file;
    closure.file = stdout;
    closure.buf = buf;
    closure.buf_size = PRINTF_FPUTCHAR_BUF_SIZE;
    closure.buf_idx = 0;

    return printf_core(fmt, &closure, args);
}

int printf(const char *fmt, ...)
{
    int count = 0;
    va_list va;
    va_start(va, fmt);

    count = vprintf(fmt, va);

    va_end(va);
    return count;
}
