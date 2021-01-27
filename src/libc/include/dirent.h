#ifndef __LIBC_INCLUDE_DIRENT_H__
#define __LIBC_INCLUDE_DIRENT_H__


#define __DIR_BUF_SIZE 4096


typedef unsigned long ino_t;
typedef long off_t;

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned char d_type;
    char d_name[256];
};


typedef struct __DIR {
    int fd;
    unsigned long last_fs_id;

    char *buf; // char buf[__DIR_BUF_SIZE];
    int buf_idx;
    int buf_size;

    struct dirent dirent;
    char dyn_alloc;
} DIR;


extern DIR *opendir(const char *pathname);
extern int closedir(DIR *d);

extern void rewinddir(DIR *d);
extern void seekdir(DIR *d, long pos);
extern long telldir(DIR *d);

extern int readdir_safe(DIR *d, struct dirent *entry);
extern int readdir_r(DIR *d, struct dirent *entry, struct dirent **result);
extern struct dirent *readdir(DIR *d);

extern int mkdir(const char *pathname, unsigned int mode);
extern int rmdir(const char *pathname);


#endif
