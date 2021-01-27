#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <crt.h>
#include <sys.h>
#include <sys/api.h>
#include <hal.h>


/*
 * Init
 */
static void init_libc()
{
    init_kprintf();
    init_libk_stop(abort);
    init_malloc();
    init_ipc();
}


/*
 * Argv
 */
static int init_argv(struct crt_entry_param *param, char ***argv_out)
{
    if (!param) {
        *argv_out = NULL;
        return 0;
    }

    int argc = param->argc;
    char **argv = calloc(argc + 1, sizeof(void *));

    char *buf = param->argv;
    for (int i = 0; i < argc; i++) {
        argv[i] = buf;
        buf += strlen(buf) + 1;
    }
    argv[argc] = NULL;

    *argv_out = argv;
    return argc;
}


/*
 * Main
 */
extern int main(int argc, char *argv[], char *envp[]);

static int call_main(int argc, char **argv, char **envp)
{
    return main(argc, argv, envp);
}


/*
 * Exit
 */
static void exit_libc(int err)
{
    unsigned long status = (long)err;

    sys_api_task_exit(status);
    syscall_thread_exit_self(0);

    panic("Should never reach here!\n");
    while (1);
}


/*
 * Entry
 */
void __libc_entry(unsigned long param)
{
    init_libc();

    char **argv = NULL;
    int argc = init_argv((void *)param, &argv);

    int err = call_main(argc, argv, NULL);
    exit_libc(err);
}


/*
 * May be overridden by arch-specific _start
 */
weak_func void _start(unsigned long param)
{
    __libc_entry(param);
}
