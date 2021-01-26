#ifndef __CONSOLE_INCLUDE_CONSOLE_H__
#define __CONSOLE_INCLUDE_CONSOLE_H__

#include <stdio.h>
#include <stdlib.h>

extern int console_read(char **o_buf, ssize_t *o_buf_size, FILE *f);

extern int console_exec(char *line, size_t line_size);

extern int exec_external(int argc, char **argv);
extern int exec_ls(int argc, char **argv);
extern int exec_cat(int argc, char **argv);
extern int exec_cd(int argc, char **argv);

#endif
