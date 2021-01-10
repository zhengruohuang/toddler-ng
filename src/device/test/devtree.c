#include <stdio.h>
#include <dirent.h>


static void list_devtree()
{
    DIR *d = NULL;
    struct dirent dent;
    int rc = 0;

    d = opendir("/sys/devtree/root");
    rc = readdir_safe(d, &dent);
    while (rc == 1) {
        kprintf("subdir: %s, fs id: %ld, type: %d\n", dent.d_name, dent.d_ino, dent.d_type);
        rc = readdir_safe(d, &dent);
    }
    closedir(d);
}

static void show_compatible()
{
    FILE *f = NULL;
    static char buf[128];

    f = fopen("/sys/devtree/root/soc/serial@7e201000/.properties/compatible", "r");
    kprintf("Serial compatible: %p\n", f);
    size_t c = fread(buf, 1, 127, f);
    do {
        for (size_t i = 0; i < c; i++) {
            if (!buf[i]) buf[i] = '\n';
        }
        buf[c] = '\0';
        kprintf("%s", buf);
        c = fread(buf, 1, 127, f);
    } while (c);
    fclose(f);
}


/*
 * Entry
 */
void test_devtree()
{
    list_devtree();
    show_compatible();
}
