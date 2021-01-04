#include <stdio.h>
#include <dirent.h>
#include <sys.h>
#include <sys/api.h>


// #include "system/include/vfs.h"
// static void test_root()
// {
//     struct ventry *ent = NULL;
//
//     kprintf("test1\n");
//     ent = vfs_acquire("/test");
//     vfs_release(ent);
//
//     kprintf("test2\n");
//     ent = vfs_acquire("/test/");
//     vfs_release(ent);
//
//     kprintf("test3\n");
//     ent = vfs_acquire("/test/test2");
//     vfs_release(ent);
//
//     kprintf("test4\n");
//     ent = vfs_acquire("/test/test2");
//     vfs_release(ent);
//
//     kprintf("test5\n");
//     ent = vfs_acquire("/test/test2/test3/test4");
//
//     kprintf("test6\n");
//     static char test_buf[128] = "Bad!\n";
//     ssize_t len = ent ? vfs_file_read(ent->vnode, 0, test_buf, 128) : 0;
//     kprintf("Test (%ld): %s\n", len, test_buf);
//     vfs_release(ent);
// }

static void test_file_api()
{
    int fd = 0;

    kprintf("test1\n");
    fd = sys_api_acquire("/test");
    sys_api_release(fd);

    kprintf("test2\n");
    fd = sys_api_acquire("/test//");
    sys_api_release(fd);

    kprintf("test3\n");
    fd = sys_api_acquire("//test/test2/");
    sys_api_release(fd);

    kprintf("test4\n");
    fd = sys_api_acquire("/test/test2/test3/test4");

    kprintf("test5\n");
    static char test_buf[128] = "Bad!\n";
    ssize_t len = fd > 0 ? sys_api_file_read(fd, test_buf, 127, 0) : 0;
    test_buf[127] = '\0';
    kprintf("FD: %d, len: %ld, message: %s\n", fd, len, test_buf);
    sys_api_release(fd);
}

static void test_stdio_file()
{
    FILE *f = NULL;

    kprintf("test1\n");
    f = fopen("/test/test2/test3/test4", "r");

    kprintf("test2\n");
    static char test_buf[128] = "Bad!\n";

    kprintf("Test message:\n");
    size_t c = fread(test_buf, 1, 127, f);
    do {
        test_buf[127] = '\0';
        kprintf("%s", test_buf);
        c = fread(test_buf, 1, 127, f);
    } while (c);

    kprintf("test3\n");
    fclose(f);

    kprintf("test4\n");
    f = fopen("/about", "r");
    kprintf("About message:\n");
    c = fread(test_buf, 1, 127, f);
    do {
        test_buf[127] = '\0';
        kprintf("%s", test_buf);
        c = fread(test_buf, 1, 127, f);
    } while (c);
    fclose(f);
}

static void test_dirent()
{
    DIR *d = NULL;
    struct dirent dent;
    int rc = 0;

    kprintf("test1\n");
    d = opendir("/test");
    kprintf("d @ %p\n", d);
    closedir(d);

    kprintf("test2\n");
    d = opendir("/");

    kprintf("test3\n");
    rc = readdir_safe(d, &dent);
    while (rc == 1) {
        kprintf("subdir: %s, fs id: %ld, type: %d\n", dent.d_name, dent.d_ino, dent.d_type);
        rc = readdir_safe(d, &dent);
    }
    closedir(d);

    kprintf("test4\n");
    d = opendir("/test");
    rc = readdir_safe(d, &dent);
    while (rc == 1) {
        kprintf("subdir: %s, fs id: %ld, type: %d\n", dent.d_name, dent.d_ino, dent.d_type);
        rc = readdir_safe(d, &dent);
    }
    closedir(d);
}

static void test_coreimg()
{
    DIR *d = NULL;
    struct dirent dent;
    int rc = 0;

    kprintf("test1\n");
    d = opendir("/sys/coreimg");
    rc = readdir_safe(d, &dent);
    while (rc == 1) {
        kprintf("subdir: %s, fs id: %ld, type: %d\n", dent.d_name, dent.d_ino, dent.d_type);
        rc = readdir_safe(d, &dent);
    }
    closedir(d);
}


/*
 * Entry
 */
void test_vfs()
{
    //test_file_api();
    //test_stdio_file();
    //test_dirent();
    test_coreimg();
}
