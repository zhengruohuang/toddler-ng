#ifndef __LIBC_INCLUDE_INTERNAL_PRINTF_H__
#define __LIBC_INCLUDE_INTERNAL_PRINTF_H__


#include <stdio.h>


/*
 * Closure
 */
struct printf_closure {
    int (*putchar)(int ch, struct printf_closure *closure);

    FILE *file;

    char *buf;
    ssize_t buf_size;
    size_t buf_idx;
};


/*
 * Token
 */
enum printf_token_types {
    PRINTF_TOKEN_NONE,
    PRINTF_TOKEN_PERCENT,
    PRINTF_TOKEN_CHAR,
    PRINTF_TOKEN_U32,
    PRINTF_TOKEN_U64,
    PRINTF_TOKEN_ULONG,
    PRINTF_TOKEN_STR,
};

struct printf_token {
    int type;
    char spec;

    union {
        u32 va_u32;
        u64 va_u64;
        ulong va_ulong;
        char *va_str;
        char va_ch;
    };

    char flags;
    int width;
    int prec;
};

extern void _printf_token(char **s, struct printf_token *token);


/*
 * Printers
 */
extern int _printf_write_char(struct printf_closure *closure, char ch);
extern int _printf_write_str(struct printf_closure *closure, const char *str);

extern int _printf_char(struct printf_closure *closure, struct printf_token *token);
extern int _printf_str(struct printf_closure *closure, struct printf_token *token);
extern int _printf_u32(struct printf_closure *closure, struct printf_token *token);
extern int _printf_u64(struct printf_closure *closure, struct printf_token *token);
extern int _printf_ulong(struct printf_closure *closure, struct printf_token *token);


#endif
