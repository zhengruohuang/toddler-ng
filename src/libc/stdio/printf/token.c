#include <internal/printf.h>


static void _printf_token_ll(char **s, struct printf_token *token)
{
    char ch = **s;
    token->spec = ch;

    switch (ch) {
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        token->type = PRINTF_TOKEN_U64;
        break;
    default:
        token->type = PRINTF_TOKEN_NONE;
        break;
    }
}

static void _printf_token_l(char **s, struct printf_token *token)
{
    char ch = **s;
    token->spec = ch;

    switch (ch) {
    case 'l':
        (*s)++;
        _printf_token_ll(s, token);
        break;
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
    case 'p':
        token->type = PRINTF_TOKEN_ULONG;
        break;
    default:
        token->type = PRINTF_TOKEN_NONE;
        break;
    }
}

void _printf_token(char **s, struct printf_token *token)
{
    char *s_copy = *s;
    char ch = **s;
    token->spec = ch;

    switch (ch) {
    case 'l':
        (*s)++;
        _printf_token_l(s, token);
        break;
    case 'd':
    case 'u':
    case 'h':
    case 'x':
    case 'b':
        token->type = PRINTF_TOKEN_U32;
        break;
    case 's':
        token->type = PRINTF_TOKEN_STR;
        break;
    case 'c':
        token->type = PRINTF_TOKEN_CHAR;
        break;
    case 'p':
        token->type = PRINTF_TOKEN_ULONG;
        break;
    case '%':
        token->type = PRINTF_TOKEN_PERCENT;
        break;
    default:
        token->type = PRINTF_TOKEN_NONE;
        break;
    }

    if (token->type == PRINTF_TOKEN_NONE) {
        *s = s_copy;
    }
}
