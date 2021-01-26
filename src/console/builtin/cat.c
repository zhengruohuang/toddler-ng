#include <stdio.h>
#include <dirent.h>

#define CAT_BUF_SIZE 512

int exec_cat(int argc, char **argv)
{
    if (argc <= 1) {
        return -1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        return -1;
    }

    char buf[CAT_BUF_SIZE + 1];
    size_t c = fread(buf, 1, CAT_BUF_SIZE, f);
    while (c) {
        buf[CAT_BUF_SIZE] = '\0';
        kprintf("%s", buf);
        c = fread(buf, 1, CAT_BUF_SIZE, f);
    };

    fclose(f);
    return 0;
}
