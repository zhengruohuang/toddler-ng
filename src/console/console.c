#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/api.h>
#include "console/include/console.h"


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

        //fprintf(f, "# Line: %s", line);
        int ret = console_exec(line, line_size);
        fprintf(f, "(ret: %d)\n", ret);
        fflush(f);
    }
}


/*
 * Main
 */
int main(int argc, char **argv)
{
    kprintf("In Console!\n");

    FILE *f = fopen("/dev/serial", "rw");
    fprintf(f, "Hello from Console!\n");
    fclose(f);

    int err = console_loop();
    panic("Console exited with code: %d!\n", err);

    while (1);
}
