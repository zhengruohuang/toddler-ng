#ifndef __SYSTEM_INCLUDE_TASK_H__
#define __SYSTEM_INCLUDE_TASK_H__


#include <kth.h>
#include <sys.h>
#include "system/include/vfs.h"


/*
 * Task
 */
struct task_file {
    int inuse;
    struct ventry *vent;
};

struct task {
    struct {
        struct task *prev;
        struct task *next;
    } list;

    const char *name;

    pid_t ppid;
    pid_t pid;

    ulong owner_uid;
    ulong run_uid;

    char *work_dir;

    int max_num_files;
    struct task_file files[32];

    cond_t exit;
    ulong exit_status;

    ref_count_t ref_count;
    rwlock_t rwlock;
};

#define access_task(pid, t) \
    for (struct task *t = acquire_task(pid); t; release_task(t), t = NULL) \
        for (int __term = 0; !__term; __term = 1)

extern struct task *acquire_task(pid_t pid);
extern void release_task(struct task *t);

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
