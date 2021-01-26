#include <hal.h>
#include <internal/printf.h>


int _printf_ulong(struct printf_closure *closure, struct printf_token *token)
{
#if (ARCH_WIDTH == 32)
    return _printf_u32(closure, token);
#elif (ARCH_WIDTH == 64)
    return _printf_u64(closure, token);
#else
    #error Unsupported ARCH_WIDTH
    return 0;
#endif
}

