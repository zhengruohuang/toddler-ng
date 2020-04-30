#include "common/include/inttypes.h"
#include "common/include/stdarg.h"
#include "common/include/abi.h"
#include "libk/include/math.h"
#include "libk/include/kprintf.h"


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
    __kputchar(ch);
    return 1;
}

static int print_string(char *str)
{
    int len = 0;
    char *s = str;

    while (*s) {
        __kputchar(*s);
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
    case 'p':
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
        printed += 20;
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
    case 'p':
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
        printed += 20;
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
enum print_token_types {
    PRINT_TOKEN_NONE,
    PRINT_TOKEN_CHAR,
    PRINT_TOKEN_U32,
    PRINT_TOKEN_U64,
    PRINT_TOKEN_ULONG,
    PRINT_TOKEN_STR,
};

struct print_params {
    union {
        u32 va_u32;
        u64 va_u64;
        ulong va_ulong;
        char *va_str;
        char va_ch;
    };

    char token;
};

static int print_token_ll(char **s, int *type, struct print_params *params)
{
    char token = **s;
    int len = 0;

    params->token = token;

    switch (token) {
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        *type = PRINT_TOKEN_U64;
        params->token = token;
        break;
    case '\0':
        *type = PRINT_TOKEN_NONE;
        len += print_string("%ll");
        break;
    default:
        *type = PRINT_TOKEN_NONE;
        len += print_string("%ll");
        len += print_char(token);
        break;
    }

    return len;
}

static int print_token_l(char **s, int *type, struct print_params *params)
{
    char token = **s;
    int len = 0;

    params->token = token;

    switch (token) {
    case 'l':
        (*s)++;
        len += print_token_ll(s, type, params);
        break;
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
    case 'p':
        *type = PRINT_TOKEN_ULONG;
        params->token = token;
        break;
    case '\0':
        *type = PRINT_TOKEN_NONE;
        len += print_string("%l");
        break;
    default:
        *type = PRINT_TOKEN_NONE;
        len += print_string("%l");
        len += print_char(token);
        break;
    }

    return len;
}

static int print_token(char **s, int *type, struct print_params *params)
{
    char token = **s;
    int len = 0;

    params->token = token;

    switch (token) {
    case 'l':
        (*s)++;
        len += print_token_l(s, type, params);
        break;
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        *type = PRINT_TOKEN_U32;
        break;
    case 's':
        *type = PRINT_TOKEN_STR;
        break;
    case 'c':
        *type = PRINT_TOKEN_CHAR;
        break;
    case 'p':
        *type = PRINT_TOKEN_ULONG;
        break;
    case '%':
        *type = PRINT_TOKEN_NONE;
        len += print_char('%');
        break;
    case '\0':
        *type = PRINT_TOKEN_NONE;
        len += print_char('%');
        break;
    default:
        *type = PRINT_TOKEN_NONE;
        len += print_char('%');
        len += print_char(token);
        break;
    }

    return len;
}

int __vkprintf(const char *fmt, va_list arg)
{
    char *s = (char *)fmt;
    int len = 0;
    int type;
    struct print_params params;

    while (*s) {
        switch (*s) {
        case '%':
            s++;
            len += print_token(&s, &type, &params);

            switch (type) {
            case PRINT_TOKEN_CHAR:
                params.va_ch = va_arg(arg, int);
                print_char(params.va_ch);
                break;
            case PRINT_TOKEN_U32:
                params.va_u32 = va_arg(arg, u32);
                print_u32(params.token, params.va_u32);
                break;
            case PRINT_TOKEN_U64:
                params.va_u64 = va_arg(arg, u64);
                print_u64(params.token, params.va_u64);
                break;
            case PRINT_TOKEN_ULONG:
                params.va_ulong = va_arg(arg, ulong);
                print_ulong(params.token, params.va_ulong);
                break;
            case PRINT_TOKEN_STR:
                params.va_str = va_arg(arg, char *);
                print_string(params.va_str);
                break;
            case PRINT_TOKEN_NONE:
            default:
                break;
            }
            break;
        default:
            len += print_char(*s);
            break;
        }

        s++;
    }

    return len;
}
