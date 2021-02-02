#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/api.h>
#include <hal.h>
#include "console/include/console.h"


/*
 * Welcome message
 */
#if (ARCH_WIDTH == 64)
static char *welcome_msg =
    "  ______          __    ____          _____ __ __\n"
    " /_  ______  ____/ ____/ / ___  _____/ ___// // /\n"
    "  / / / __ )/ __  / __  / / _ )/ ___/ __ )/ // /_\n"
    " / / / /_/ / /_/ / /_/ / /  __/ /  / /_/ /__  __/\n"
    "/_/  (____/(__,_/(__,_/_/(___/_/   (____/  /_/   \n"
;
#else
static char *welcome_msg =
    "  ______          __    ____         \n"
    " /_  ______  ____/ ____/ / ___  _____\n"
    "  / / / __ )/ __  / __  / / _ )/ ___/\n"
    " / / / /_/ / /_/ / /_/ / /  __/ /    \n"
    "/_/  (____/(__,_/(__,_/_/(___/_/     \n"
;
#endif


/*
 * Console loop
 */
static int console_loop()
{
    FILE *f = fopen("/dev/serial", "rw");

    while (1) {
        char cwd[512];
        sys_api_task_get_work_dir(cwd, 512);

        fprintf(f, "%s# ", cwd);
        fflush(f);

        char *line = NULL;
        ssize_t line_size = 0;
        int err = console_read(&line, &line_size, f);
        if (err) {
            return -1;
        }

        int ret = console_exec(line, line_size);
        fprintf(f, "(ret: %d)\n", ret);
        fflush(f);

        free(line);
    }

    fclose(f);
}


/*
 * Main
 */
int main(int argc, char **argv)
{
    kprintf("In Console!\n");

    FILE *f = fopen("/dev/serial", "rw");
    fprintf(f, "Hello from Console!\n");
    fprintf(f, "%s", welcome_msg);
    fclose(f);

    int err = console_loop();
    panic("Console exited with code: %d!\n", err);

    while (1);
}
