#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "common/include/arch.h"
#include "loader/include/lib.h"
#include "loader/include/lprintf.h"


/*
 * Helpers
 */
static void u32_to_hex(u32 num, char *buf)
{
    for (int i = 0; i < 8; i++) {
        int digit = (int)((num << (i << 2)) >> 28);
        buf[i] = digit >= 10 ?
                (char)(digit - 10 + 'a') :
                (char)(digit + '0');
    }
}

static void u32_to_dec(u32 num, char *buf)
{
    u32 dividers[] = { 1000000000u, 100000000u, 10000000u,
        1000000u, 100000u, 10000u, 1000u, 100u, 10u, 1u };
    u32 q = 0;

    for (int i = 0; i < 10; i++) {
        div_u32(num, dividers[i], &q, &num);
        buf[i] = '0' + q;
    }
}

static void u64_to_hex(u64 num, char *buf)
{
    for (int i = 0; i < 16; i++) {
        u64 digit = (int)((num << (i << 2)) >> 60);
        buf[i] = digit >= 10 ?
                (char)(digit - 10 + 'a') :
                (char)(digit + '0');
    }
}

static void u64_to_dec(u64 num, char *buf)
{
    u64 dividers[] = { 10000000000000000000ull,
        1000000000000000000ull, 100000000000000000ull,
        10000000000000000ull,   1000000000000000ull,
        100000000000000ull,     10000000000000ull,
        1000000000000ull,       100000000000ull,
        10000000000ull, 1000000000ull, 100000000ull, 10000000ull,
        1000000ull, 100000ull, 10000ull, 1000ull, 100ull, 10ull, 1ull };
    u64 q = 0;

    for (int i = 0; i < 20; i++) {
        div_u64(num, dividers[i], &q, &num);
        buf[i] = '0' + q;
    }
}


/*
 * Printing items
 */
static int print_char(char ch)
{
    lputchar(ch);
    return 1;
}

static int print_string(char *str)
{
    int len = 0;
    char *s = str;

    while (*s) {
        lputchar(*s);
        s++;
        len++;
    }

    return len;
}

static int print_u32(char fmt, u32 num)
{
    int i;
    int started = 0, printed = 0;
    char buf[10];

    switch (fmt) {
    case 'b':
        for (i = 31; i >= 0; i--) {
            print_char(num & (0x1 << i) ? '1' : '0');
        }
        break;
    case 'x':
        print_string("0x");
    case 'h':
        if (!num) {
            print_char('0');
        } else {
            u32_to_hex(num, buf);
            for (i = 0; i < 8; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { print_char(buf[i]); printed++; }
            }
        }
        break;
    case 'd':
        if (num & (0x1 << 31)) {
            print_char('-');
            num = ~num + 1;
        }
    case 'u':
        if (!num) {
            print_char('0');
            break;
        } else {
            u32_to_dec(num, buf);
            for (i = 0; i < 10; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { print_char(buf[i]); printed++; }
            }
        }
        break;
    default:
        print_string("__unknown_format_");
        print_char(fmt);
        print_string("__");
        break;
    }

    return printed;
}

static int print_u64(char fmt, u64 num)
{
    int i;
    int started = 0, printed = 0;
    char buf[20];

    switch (fmt) {
    case 'b':
        for (i = 63; i >= 0; i--) {
            print_char(num & (0x1 << i) ? '1' : '0');
        }
        break;
    case 'x':
        print_string("0x");
    case 'h':
        if (!num) {
            print_char('0');
        } else {
            u64_to_hex(num, buf);
            for (i = 0; i < 16; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { print_char(buf[i]); printed++; }
            }
        }
        break;
    case 'd':
        if (num & (0x1ull << 63)) {
            print_char('-');
            num = ~num + 0x1ull;
        }
    case 'u':
        if (!num) {
            print_char('0');
            break;
        } else {
            u64_to_dec(num, buf);
            for (i = 0; i < 20; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { print_char(buf[i]); printed++; }
            }
        }
        break;
    default:
        print_string("__unknown_format_");
        print_char(fmt);
        print_string("__");
        break;
    }

    return printed;
}

static int print_ulong(char fmt, ulong num)
{
#if (ARCH_WIDTH == 32)
    return print_u32(fmt, (u32)num);
#elif (ARCH_WIDTH == 64)
    return print_u64(fmt, (u64)num);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}


/*
 * Token handling
 */
static int print_token_ll(char **s, va_list *arg)
{
    char token = **s;
    u64 va_u64 = 0;
    int len = 0;

    switch (token) {
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        va_u64 = va_arg(*arg, u64);
        len += print_u64(token, va_u64);
        break;
    case '\0':
        len += print_string("%ll");
        break;
    default:
        len += print_string("%ll");
        len += print_char(token);
        break;
    }

    return len;
}

static int print_token_l(char **s, va_list *arg)
{
    char token = **s;
    ulong va_ulong = 0;
    int len = 0;

    switch (token) {
    case 'l':
        (*s)++;
        len += print_token_ll(s, arg);
        break;
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        va_ulong = va_arg(*arg, ulong);
        len += print_ulong(token, va_ulong);
        break;
    case '\0':
        len += print_string("%l");
        break;
    default:
        len += print_string("%l");
        len += print_char(token);
        break;
    }

    return len;
}

static int print_token(char **s, va_list *arg)
{
    char token = **s;

    u32 va_u32 = 0;
    ulong va_ulong = 0;
    char *va_str = NULL;
    char va_ch = 0;

    int len = 0;

    switch (token) {
    case 'l':
        (*s)++;
        len += print_token_l(s, arg);
        break;
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        va_u32 = va_arg(*arg, u32);
        len += print_u32(token, va_u32);
        break;
    case 's':
        va_str = va_arg(*arg, char *);
        len += print_string(va_str);
        break;
    case 'c':
        va_ch = va_arg(*arg, int);
        len += print_char(va_ch);
        break;
    case 'p':
        va_ulong = va_arg(*arg, ulong);
        len += print_ulong('x', va_ulong);
        break;
    case '%':
        len += print_char('%');
        break;
    case '\0':
        len += print_char('%');
        break;
    default:
        len += print_char('%');
        len += print_char(token);
        break;
    }

    return len;
}

int vlprintf(const char *fmt, va_list arg)
{
    char *s = (char *)fmt;
    int len = 0;

    while (*s) {
        switch (*s) {
        case '%':
            s++;
            len += print_token(&s, &arg);
            break;
        default:
            len += print_char(*s);
            break;
        }

        s++;
    }

    return len;
}
