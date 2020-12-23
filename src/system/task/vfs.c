#include "common/include/inttypes.h"
#include "common/include/atomic.h"
#include "common/include/refcount.h"
#include "system/include/kprintf.h"
#include "system/include/thread.h"
#include "libsys/include/syscall.h"


enum vfs_ops {
    VFS_OP_DUMMY,

    VFS_OP_MOUNT,
    VFS_OP_UNMOUNT,

    VFS_OP_ACCESS,
    VFS_OP_RELEASE,
    VFS_OP_MOVE,

    VFS_OP_FILE_OPEN,
    VFS_OP_FILE_READ,
    VFS_OP_FILE_WRITE,
    VFS_OP_FILE_FLUSH,
    VFS_OP_FILE_CREATE,
    VFS_OP_FILE_TRUNCATE,
    VFS_OP_FILE_SYNC,

    VFS_OP_FILE_LINK,
    VFS_OP_FILE_UNLINK,

    VFS_OP_DIR_OPEN,
    VFS_OP_DIR_READ,
    VFS_OP_DIR_CREATE,
    VFS_OP_DIR_REMOVE,
    VFS_OP_DIR_SYNC,

    VFS_OP_CTRL_OPNEN,

    NUM_VFS_OPS,

    // getattr
    // mknod
    // chmod
    // chown
    // statfs
    // setxattr
    // getxattr
    // listxattr
    // remove xattr
    // init
    // distory
    // access
    // fgetattr
    // lock
    // utimes
    // bmap
    // symlink
    // readlink
};

struct ventry;

struct mount_point {
    struct ventry *entry;
    char *name;

    ulong pid;
    ulong ops[NUM_VFS_OPS];
};

struct ventry {
    struct ventry *parent;
    struct ventry *child;
    struct ventry *sibling;

    char *name;
    struct mount_point *mount;

    ref_count_t ref_count;
};


int vfs_mount(const char *path)
{
}

int vfs_unmount(struct ventry *node)
{
}
