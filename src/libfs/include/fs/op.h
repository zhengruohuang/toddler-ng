#ifndef __LIBFS_INCLUDE_FS_OP__
#define __LIBFS_INCLUDE_FS_OP__


enum vfs_ops {
    VFS_OP_INVALID,

    VFS_OP_MOUNT,
    VFS_OP_UNMOUNT,

    VFS_OP_LOOKUP,
    VFS_OP_FORGET,

    VFS_OP_ACQUIRE,
    VFS_OP_RELEASE,
    VFS_OP_ACCESS,
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

    VFS_OP_SYMLINK_CREATE,
    VFS_OP_SYMLINK_READ,

    VFS_OP_DIR_OPEN,
    VFS_OP_DIR_READ,
    VFS_OP_DIR_CREATE,
    VFS_OP_DIR_REMOVE,
    VFS_OP_DIR_SYNC,

    VFS_OP_CTRL_OPEN,
    VFS_OP_CTRL_QUERY,
    VFS_OP_CTRL_READ,
    VFS_OP_CTRL_WRITE,

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
    // destroy
    // fgetattr
    // lock
    // utimes
    // bmap
    // symlink
    // readlink
};


#endif
