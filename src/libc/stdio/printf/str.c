#include <internal/printf.h>


int _printf_char(struct printf_closure *closure, struct printf_token *token)
{
    return closure->putchar(token->va_ch, closure);
}

int _printf_str(struct printf_closure *closure, struct printf_token *token)
{
    int len = 0;

    for (char *s = token->va_str; *s; s++, len++) {
        int c = closure->putchar(*s, closure);
        if (!c) {
            break;
        }
    }

    return len;
}


int _printf_write_char(struct printf_closure *closure, char ch)
{
    return closure->putchar(ch, closure);
}

int _printf_write_str(struct printf_closure *closure, const char *str)
{
    int len = 0;

    for (const char *s = str; *s; s++, len++) {
        int c = closure->putchar(*s, closure);
        if (!c) {
            break;
        }
    }

    return len;
}
