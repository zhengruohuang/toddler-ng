#include <internal/printf.h>
#include "libk/include/math.h"


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
    u64 dividers[] = {
        10000000000000000000ull,
        1000000000000000000ull, 100000000000000000ull,
        10000000000000000ull,   1000000000000000ull,
        100000000000000ull,     10000000000000ull,
        1000000000000ull,       100000000000ull,
        10000000000ull,
        1000000000ull,          100000000ull,
        10000000ull,            1000000ull,
        100000ull, 10000ull,    1000ull, 100ull,
        10ull, 1ull
    };
    u64 q = 0;

    for (int i = 0; i < 20; i++) {
        div_u64(num, dividers[i], &q, &num);
        buf[i] = '0' + q;
    }
}

int _printf_u64(struct printf_closure *closure, struct printf_token *token)
{
    int i;
    int started = 0;
    char buf[20];

    int len = 0;
    char fmt = token->spec;
    u64 num = token->va_u64;

    switch (fmt) {
    case 'b':
        for (i = 63; i >= 0; i--) {
            _printf_write_char(closure, num & (0x1 << i) ? '1' : '0');
        }
        break;
    case 'p':
    case 'x':
        _printf_write_str(closure, "0x");
    case 'h':
        if (!num) {
            _printf_write_char(closure, '0');
        } else {
            u64_to_hex(num, buf);
            for (i = 0; i < 16; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { _printf_write_char(closure, buf[i]); len++; }
            }
        }
        break;
    case 'd':
        if (num & (0x1ull << 63)) {
            _printf_write_char(closure, '-');
            num = ~num + 0x1ull;
        }
    case 'u':
        if (!num) {
            _printf_write_char(closure, '0');
            break;
        } else {
            u64_to_dec(num, buf);
            for (i = 0; i < 20; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { _printf_write_char(closure, buf[i]); len++; }
            }
        }
        break;
    default:
        break;
    }

    return len;
}
