#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common/include/compiler.h"


/*
 * Init
 */
static void init_libc()
{
    init_kprintf();
    init_libk_stop(abort);
    init_malloc();
}


/*
 * Main
 */
extern int main(int argc, char *argv[], char *envp[]);

static int call_main()
{
    return main(0, NULL, NULL);
}


/*
 * Exit
 */
static void exit_libc(int err)
{
}


/*
 * Entry
 */
void __libc_entry()
{
    init_libc();
    int err = call_main();
    exit_libc(err);
}


/*
 * May be overridden by arch-specific _start
 */
weak_func void _start(unsigned long param)
{
    __libc_entry();
}
