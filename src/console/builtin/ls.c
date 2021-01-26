#include <stdio.h>
#include <dirent.h>
#include <sys/api.h>

int exec_ls(int argc, char **argv)
{
    char *pathname = ".";
    if (argc > 1 && argv[1]) {
        pathname = argv[1];
    }

    DIR *d = opendir(pathname);
    if (!d) {
        return -1;
    }

    struct dirent dent;
    int rc = readdir_safe(d, &dent);
    while (rc == 1) {
        char prefix = ' ';
        switch (dent.d_type) {
        case VFS_NODE_DIR:
            prefix = '+';
            break;
        case VFS_NODE_DEV:
            prefix = '*';
            break;
        case VFS_NODE_PIPE:
            prefix = '|';
        default:
            break;
        }

        kprintf("%c %s (%lu)\n", prefix, dent.d_name, dent.d_ino);
        rc = readdir_safe(d, &dent);
    }

    closedir(d);
    return 0;
}
