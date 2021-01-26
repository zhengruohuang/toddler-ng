#include <stdio.h>
#include <dirent.h>
#include <sys/api.h>

int exec_cd(int argc, char **argv)
{
    char *pathname = "/";
    if (argc > 1 && argv[1]) {
        pathname = argv[1];
    }

    DIR *d = opendir(pathname);
    if (!d) {
        return -1;
    }

    sys_api_task_set_work_dir(pathname);

    closedir(d);
    return 0;
}
