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
static int task_salloc_ctor(void *obj, size_t size)
{
    memzero(obj, size);

    struct task *t = obj;
    t->work_dir = malloc(1024);

    return 0;
}

static int task_salloc_dtor(void *obj, size_t size)
{
    struct task *t = obj;

    if (t->work_dir) {
        free(t->work_dir);
    }

    if (t->name) {
        free(t->name);
    }

    return 0;
}

static salloc_obj_t task_salloc = SALLOC_CREATE(sizeof(struct task), 0,
                                                task_salloc_ctor,
                                                task_salloc_dtor);

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

    strcpy(t->work_dir, "/");
    t->state = TASK_STATE_ENTER;

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
static void _create_init_task(pid_t pid, const char *name)
{
    struct task *t = new_task();
    t->ppid = 0;
    t->pid = pid;
    t->name = strdup(name);
    t->state = TASK_STATE_RUN;
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
    char *abs_path = task_abs_path(ppid, path);
    //kprintf("ELF abs path: %s\n", abs_path);

    unsigned long entry = 0, free_start = 0;
    int err = exec_elf(pid, abs_path, &entry, &free_start);
    free(abs_path);

    if (err) {
        kprintf("Unable to load ELF @ %s\n", path);
        task_exit(pid, -1ul);
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

    syscall_vm_free((unsigned long)param);

    // Start
    t->state = TASK_STATE_RUN;
    tid_t tid = syscall_thread_create_cross(pid, entry, entry_struct_addr);
    if (!tid) {
        t->state = TASK_STATE_ENTER;
        kprintf("Unable to run new task @ %s\n", path);
        task_exit(pid, -1ul);
        return -1;
    }

    return pid;
}

int task_detach(pid_t pid, unsigned long status)
{
    int err = -1;

    access_task(pid, task) {
        rwlock_exclusive_w(&task->rwlock) {
            if (task->state == TASK_STATE_RUN) {
                task->state = TASK_STATE_DETACH;
                err = 0;
            }
        }

        if (!err) {
            task->exit_status = status;
            cond_signal(&task->exit);
        }
    }

    return err;
}

int task_exit(pid_t pid, unsigned long status)
{
    int err = -1;

    access_task(pid, task) {
        rwlock_exclusive_w(&task->rwlock) {
            if (task->state != TASK_STATE_EXIT) {
                task->state = TASK_STATE_EXIT;
                err = 0;
            }
        }

        if (!err) {
            task->exit_status = status;
            cond_signal(&task->exit);

            // Ask kernel to stop the process
            err = syscall_process_exit(pid, status);
        }
    }

    return err;
}

int task_cleanup(pid_t pid)
{
    // Process stopped by kernel, cleanup all memory used

    int err = -1;

    struct task *t = NULL;
    access_task(pid, task) {
        t = task;
    }

    //kprintf("To delete task\n");
    del_task(t);

    // Ask kernel to free all kernel data
    err = syscall_process_recycle(pid);

    return err;
}

int task_crash(pid_t pid)
{
    // Process stopped by kernel, cleanup all memory used

    int err = -1;

    struct task *t = NULL;
    access_task(pid, task) {
        t = task;
    }

    del_task(t);

    // Ask kernel to free all kernel data
    err = syscall_process_recycle(pid);

    return err;
}


/*
 * Working dir
 */
char *task_abs_path(pid_t pid, const char *pathname)
{
    // Skip leading empty chars
    while (*pathname == ' ' || *pathname == '\t') {
        pathname++;
    }

    // Already abs path
    if (*pathname == '/') {
        return strdup(pathname);
    }

    // Abs path
    char *abs_pathname = NULL;
    access_task(pid, task) {
        char cwd_buf[512 + 1];
        rwlock_exclusive_r(&task->rwlock) {
            strncpy(cwd_buf, task->work_dir, 510);
            cwd_buf[512] = '\0';
        }

        size_t cwd_len = strlen(cwd_buf);
        if (cwd_buf[cwd_len - 1] != '/') {
            cwd_buf[cwd_len] = '/';
            cwd_buf[cwd_len + 1] = '\0';
        }

        abs_pathname = strcatcpy(cwd_buf, pathname);
    }

    return abs_pathname;
}

int task_set_work_dir(pid_t pid, const char *pathname)
{
    char *abs_pathname = task_abs_path(pid, pathname);

    // Acquire
    struct ventry *vent = vfs_acquire(abs_pathname);
    free(abs_pathname);
    if (!vent || !vent->vnode) {
        return -1;
    }

    // Find real path
    char *real_pathname = malloc(1024);
    int err = vfs_real_path(vent, real_pathname, 1024);
    if (err) {
        free(real_pathname);
        return -1;
    }

    // Set work dir
    access_task(pid, task) {
        rwlock_exclusive_w(&task->rwlock) {
            strcpy(task->work_dir, real_pathname);
        }
    }

    free(real_pathname);
    return 0;
}

int task_get_work_dir(pid_t pid, char *buf, size_t buf_size)
{
    int err = -1;

    // Get work dir
    access_task(pid, task) {
        rwlock_exclusive_r(&task->rwlock) {
            size_t len = strlen(task->work_dir);
            if (buf && len < buf_size) {
                strcpy(buf, task->work_dir);
                err = 0;
            }
        }
    }

    return err;
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
