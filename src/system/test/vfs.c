#include "common/include/inttypes.h"
#include "libsys/include/syscall.h"
#include "libsys/include/ipc.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "system/include/vfs.h"


static char test_buf[128] = "Bad!\n";

static void test_root()
{
    struct ventry *ent = NULL;

    kprintf("test1\n");
    ent = vfs_acquire("/test");
    vfs_release(ent);

    kprintf("test2\n");
    ent = vfs_acquire("/test/");
    vfs_release(ent);

    kprintf("test3\n");
    ent = vfs_acquire("/test/test2");
    vfs_release(ent);

    kprintf("test4\n");
    ent = vfs_acquire("/test/test2");
    vfs_release(ent);

    kprintf("test5\n");
    ent = vfs_acquire("/test/test2/test3/test4");

    kprintf("test6\n");
    ssize_t len = ent ? vfs_file_read(ent->vnode, 0, test_buf, 128) : 0;
    kprintf("Test (%ld): %s\n", len, test_buf);
    vfs_release(ent);
}


/*
 * Entry
 */
void test_vfs()
{
    test_root();
}
