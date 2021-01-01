#include <stdio.h>
#include <assert.h>

static FILE _stdin = { .fd = 0 };
static FILE _stdout = { .fd = 1 };
static FILE _stderr = { .fd = 2 };

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;
