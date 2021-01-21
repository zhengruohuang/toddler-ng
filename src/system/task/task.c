#include <kth.h>
#include <sys.h>
#include <stdlib.h>
#include <string.h>
#include <atomic.h>
#include <assert.h>
#include <crt.h>

#include "common/include/inttypes.h"
#include "libk/include/mem.h"
#include "system/include/vfs.h"
#include "system/include/task.h"


/*
 * Access
 */
struct task_list {
    unsigned long count;
    struct task *first;

    rwlock_t rwlock;
};

static struct task_list tasks = { .count = 0, .first = NULL, .rwlock = RWLOCK_INITIALIZER };

struct task *acquire_task(pid_t pid)
{
    struct task *at = NULL;

    rwlock_exclusive_r(&tasks.rwlock) {
        for (struct task *t = tasks.first; t; t = t->list.next) {
            if (t->pid == pid) {
                ref_count_inc(&t->ref_count);
                at = t;
                break;
            }
        }
    }

    return at;
}

void release_task(struct task *t)
{
    ref_count_dec(&t->ref_count);
}


/*
 * Allocation
 */
static salloc_obj_t task_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct task));

static struct task *new_task()
{
    struct task *t = salloc(&task_salloc);
    if (!t) {
        return NULL;
    }

    t->max_num_files = 32;
    for (int i = 0; i < 3; i++) {
        t->files[i].inuse = 1;
    }

    rwlock_exclusive_w(&tasks.rwlock) {
        t->list.prev = NULL;
        t->list.next = tasks.first;
        if (tasks.first) {
            tasks.first->list.prev = t;
        }
        tasks.first = t;
        tasks.count++;
    }

    return t;
}

static int del_task(struct task *task)
{
    panic_if(!ref_count_is_zero(&task->ref_count), "Inconsistent ref count!\n");

    rwlock_exclusive_w(&tasks.rwlock) {
        if (task->list.prev) task->list.prev->list.next = task->list.next;
        if (task->list.next) task->list.next->list.prev = task->list.prev;
        if (tasks.first == task) tasks.first = task->list.next;
        tasks.count--;
    }

    sfree(task);
    return 0;
}


/*
 * Init
 */
static inline void _create_init_task(pid_t pid, const char *name)
{
    struct task *t = new_task();
    t->ppid = 0;
    t->pid = pid;
    t->name = name;
}

void init_task()
{
    _create_init_task(0, "kernel.elf");
    _create_init_task(syscall_get_tib()->pid, "system.elf");
}


/*
 * Task create and exit
 */
pid_t task_create(pid_t ppid, int type, int argc, char **argv, const char *stdio[3])
{
    if (!argc || !argv || !argv[0]) {
        return -1;
    }

    // Create process
    pid_t pid = syscall_process_create(type);
    if (!pid) {
        return -1;
    }

    // Load ELF
    const char *path = argv[0];
    kprintf("path: %s\n", path);
    unsigned long entry = 0, free_start = 0;
    int err = exec_elf(pid, path, &entry, &free_start);
    if (err) {
        return -1;
    }

    // Set up task struct
    struct task *t = new_task();
    t->ppid = ppid;
    t->pid = pid;
    t->name = strdup(path);

    // Set up args
    unsigned long entry_struct_addr = align_up_vaddr(free_start + PAGE_SIZE, PAGE_SIZE);
    size_t param_size = sizeof(struct crt_entry_param);
    for (int i = 0; i < argc; i++) {
        param_size += argv[i] ? (strlen(argv[i]) + 1) : 1;
    }

    struct crt_entry_param *param = (void *)syscall_vm_map_cross(pid, entry_struct_addr, param_size, 0);
    param->argc = argc;
    param->argv_size = param_size;

    char *argv_buf = param->argv;
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            strcpy(argv_buf, argv[i]);
            argv_buf += strlen(argv[i]) + 1;
        } else {
            *argv_buf = '\0';
            argv_buf++;
        }
    }

    // Start
    tid_t tid = syscall_thread_create_cross(pid, entry, entry_struct_addr);
    if (!tid) {
        return -1;
    }

    return pid;
}

int task_exit(pid_t pid, int status)
{
    struct task *t = NULL;
    access_task(pid, task) {
        t = task;
    }

    if (t) {
        del_task(t);
    }

    return 0;
}


/*
 * Task file descriptors
 */
int task_alloc_fd(pid_t pid, struct ventry *vent)
{
    int fd = -1;

    access_task(pid, task) {
        rwlock_exclusive_w(&task->rwlock) {
            for (int i = 0; i < task->max_num_files; i++) {
                struct task_file *f = &task->files[i];
                if (!f->inuse) {
                    f->inuse = 1;
                    f->vent = vent;
                    fd = i;
                    break;
                }
            }
        }
    }

    return fd;
}

int task_free_fd(pid_t pid, int fd)
{
    int err = -1;

    access_task(pid, task) {
        rwlock_exclusive_w(&task->rwlock) {
            if (fd < task->max_num_files) {
                struct task_file *f = &task->files[fd];
                if (f->inuse) {
                    f->inuse = 0;
                    f->vent = NULL;
                    err = 0;
                }
            }
        }
    }

    return err;
}

struct ventry *task_lookup_fd(pid_t pid, int fd)
{
    struct ventry *vent = NULL;

    access_task(pid, task) {
        rwlock_exclusive_r(&task->rwlock) {
            if (fd < task->max_num_files) {
                struct task_file *f = &task->files[fd];
                if (f->inuse) {
                    vent = f->vent;
                }
            }
        }
    }

    return vent;
}

int task_set_fd(pid_t pid, int fd, struct ventry *vent)
{
    int err = -1;

    access_task(pid, task) {
        rwlock_exclusive_w(&task->rwlock) {
            if (fd < task->max_num_files) {
                struct task_file *f = &task->files[fd];
                if (f->inuse) {
                    f->vent = vent;
                    err = 0;
                }
            }
        }
    }

    return err;
}
