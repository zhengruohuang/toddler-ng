#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "console/include/console.h"


/*
 * Built-in commands
 */
typedef int (*cmd_exec_t)(int argc, char **argv);

struct builtin_exec {
    const char *cmd;
    cmd_exec_t exec;
};

static struct builtin_exec builtin_execs[] = {
    { .cmd = "ls", exec_ls },
    { .cmd = "cat", exec_cat },
    { .cmd = "cd", exec_cd },
    { .cmd = "stats", exec_stats },
    { .cmd = "test", exec_test },
    { .cmd = NULL, NULL }
};


/*
 * Parse a line
 */
static int is_space(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n';
}

int console_exec(char *line, size_t line_size)
{
    // argc
    int argc = 0;
    for (char *cur = line; *cur; ) {
        while (is_space(*cur)) {
            cur++;
        }

        if (!*cur) {
            break;
        }
        argc++;

        while (*cur && !is_space(*cur)) {
            cur++;
        }
    }

    // empty command
    if (!argc) {
        return 0;
    }

    // argv
    int arg_idx = 0;
    char **argv = malloc(sizeof(char *) * (argc + 1));
    for (char *cur = line; *cur; ) {
        while (is_space(*cur)) {
            *cur = '\0';
            cur++;
        }

        if (!*cur) {
            break;
        }
        argv[arg_idx++] = cur;

        while (*cur && !is_space(*cur)) {
            cur++;
        }
    }
    argv[argc] = NULL;

    // exec
    cmd_exec_t exec = exec_external;
    for (struct builtin_exec *r = builtin_execs; r && r->cmd; r++) {
        if (!strcmp(r->cmd, argv[0])) {
            exec = r->exec;
            break;
        }
    }

    int err = exec(argc, argv);

    // Done
    free(argv);
    return err;
}
