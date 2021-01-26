#include <internal/printf.h>
#include "libk/include/math.h"


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

int _printf_u32(struct printf_closure *closure, struct printf_token *token)
{
    int i;
    int started = 0;
    char buf[10];

    int len = 0;
    char fmt = token->spec;
    u64 num = token->va_u32;

    switch (fmt) {
    case 'b':
        for (i = 31; i >= 0; i--) {
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
            u32_to_hex(num, buf);
            for (i = 0; i < 8; i++) {
                if (buf[i] != '0') started = 1;
                if (started) { _printf_write_char(closure, buf[i]); len++; }
            }
        }
        break;
    case 'd':
        if (num & (0x1 << 31)) {
            _printf_write_char(closure, '-');
            num = ~num + 1;
        }
    case 'u':
        if (!num) {
            _printf_write_char(closure, '0');
            break;
        } else {
            u32_to_dec(num, buf);
            for (i = 0; i < 10; i++) {
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
