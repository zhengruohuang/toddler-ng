#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/api.h>


static inline int path_push(char *buf, size_t buf_size, const char *name)
{
    size_t old_len = strlen(buf);
    size_t name_len = strlen(name);
    if (old_len + name_len + 2 > buf_size) {
        return -1;
    }

    buf[old_len] = '/';
    char *copy = &buf[old_len + 1];
    memcpy(copy, name, name_len);
    copy[name_len] = '\0';

    return 0;
}

static inline void path_pop(char *buf)
{
    for (size_t i = strlen(buf); i; i--) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            return;
        }
    }
}


static inline void display(const char *name, int type, int level)
{
    char prefix = ' ';
    switch (type) {
    case VFS_NODE_DIR:
        prefix = '+';
        break;
    case VFS_NODE_DEV:
        prefix = '*';
        break;
    case VFS_NODE_PIPE:
        prefix = '|';
        break;
    case VFS_NODE_SYMLINK:
        prefix = '~';
        break;
    case VFS_NODE_FILE:
    default:
        prefix = '-';
        break;
    }

    for (int i = 0; i < level - 1; i++) {
        kprintf("|  ");
    }
    kprintf("|- %c %s\n", prefix, name);
}

static inline int file(const char *filename, int type, int level)
{
    return 0;
}

static int tree(char *path, size_t buf_size,
                const char *dirname, int level, int max_level)
{
    DIR *d = opendir(path);
    if (!d) {
        return -1;
    }

    int err = 0;

    struct dirent dent;
    int rc = readdir_safe(d, &dent);
    while (!err && rc == 1) {
        display(dent.d_name, dent.d_type, level);

        switch (dent.d_type) {
        case VFS_NODE_DIR:
            if (level < max_level) {
                err = path_push(path, buf_size, dent.d_name);
                if (!err) {
                    err = tree(path, buf_size, dent.d_name, level + 1, max_level);
                    path_pop(path);
                }
            }
            break;
        case VFS_NODE_FILE:
            err = file(dent.d_name, dent.d_type, level + 1);
            break;
        default:
            break;
        }

        rc = readdir_safe(d, &dent);
    }

    closedir(d);
    return err;
}

int exec_tree(int argc, char **argv)
{
    int depth = 2;
    for (int i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], "-d")) {
            depth = atoi(argv[i + 1]);
        }
    }

    static char path[512];
    sys_api_task_get_work_dir(path, 512);
    kprintf("%s\n", path);

    strcpy(path, ".");
    int err = tree(path, 512, ".", 1, depth);
    return err;
}
