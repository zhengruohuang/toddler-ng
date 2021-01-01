#include <stdlib.h>


/*
 * Exit destructors
 */
struct exit_dtor {
    struct exit_dtor *next;
    int has_arg;
    union {
        void (*func1)(void);
        void (*func2)(int, void *);
    };
    void *arg;
};

static salloc_obj_t exit_dtor_salloc = SALLOC_CREATE_DEFAULT(sizeof(struct exit_dtor));

static struct exit_dtor *exit_dtors = NULL;

static inline void invoke_exit_dtors(int status)
{
    for (struct exit_dtor *s = exit_dtors; s; s = s->next) {
        if (s->has_arg) {
            s->func2(status, s->arg);
        } else {
            s->func1();
        }
    }
}

static inline void push_exit_dtor(struct exit_dtor *s)
{
    s->next = exit_dtors;
    exit_dtors = s;
}

int atexit(void (*func)(void))
{
    struct exit_dtor *s = salloc(&exit_dtor_salloc);
    if (!s) {
        return -1;
    }

    s->has_arg = 0;
    s->func1 = func;
    push_exit_dtor(s);
    return 0;
}

int on_exit(void (*func)(int, void *), void *arg)
{
    struct exit_dtor *s = salloc(&exit_dtor_salloc);
    if (!s) {
        return -1;
    }

    s->has_arg = 1;
    s->func2 = func;
    s->arg = arg;
    push_exit_dtor(s);
    return 0;
}


/*
 * Exit
 */
void _exit(int status)
{
    // TODO: close all files
}

void exit(int status)
{
    invoke_exit_dtors(status);
    _exit(status);
}


/*
 * Abort
 */
void abort()
{
    while (1);
}
