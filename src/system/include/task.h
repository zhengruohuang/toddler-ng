#ifndef __SYSTEM_INCLUDE_TASK_H__
#define __SYSTEM_INCLUDE_TASK_H__


#include <sys.h>
#include "system/include/vfs.h"


/*
 * Task
 */
extern void init_task();

extern pid_t task_create(pid_t ppid, int type, int argc, char **argv, const char *stdio[3]);
extern int task_exit(pid_t pid, int status);


extern int task_alloc_fd(pid_t pid, struct ventry *vent);
extern int task_free_fd(pid_t pid, int fd);
extern struct ventry *task_lookup_fd(pid_t pid, int fd);
extern int task_set_fd(pid_t pid, int fd, struct ventry *vent);


/*
 * Exec
 */
extern int exec_elf(pid_t pid, const char *path, unsigned long *entry, unsigned long *free_start);


/*
 * Startup
 */
extern void startup();


#endif
