#ifndef __SYSTEM_INCLUDE_EXEC_H__
#define __SYSTEM_INCLUDE_EXEC_H__

#include <sys.h>

extern int exec_elf(pid_t pid, const char *path, unsigned long *entry, unsigned long *free_start);

#endif
