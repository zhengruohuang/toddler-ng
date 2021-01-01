#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/api.h>


static salloc_obj_t dir_salloc = SALLOC_CREATE_DEFAULT(sizeof(DIR));


/*
 * Open/Close
 */
DIR *opendir(const char *pathname)
{
    int fd = sys_api_acquire(pathname);
    if (fd < 0) {
        return NULL;
    }

    int err = sys_api_dir_open(fd, 0);
    if (err) {
        return NULL;
    }

    DIR *d = salloc(&dir_salloc);
    if (!d) {
        return NULL;
    }

    d->fd = fd;
    return d;
}

int closedir(DIR *d)
{
    if (!d) {
        return -1;
    }

    int err = sys_api_release(d->fd);
    if (err) {
        return err;
    }

    sfree(d);
    return 0;
}


/*
 * Pos
 */
void rewinddir(DIR *d)
{
    d->last_fs_id = 0;
    d->buf_idx = 0;
}

void seekdir(DIR *d, long pos)
{
    d->last_fs_id = pos;
    d->buf_idx = 0;
}

long telldir(DIR *d)
{
    return d->last_fs_id;
}


/*
 * Read
 */
int readdir_safe(DIR *d, struct dirent *entry)
{
    if (!d) {
        return -1;
    }

    if (d->buf_idx >= d->buf_size) {
        d->buf_idx = 0;
        ssize_t read_count = sys_api_dir_read(d->fd, (void *)d->buf,
                                              __DIR_BUF_SIZE, d->last_fs_id);

        // error or end of stream
        if (read_count <= 0) {
            d->buf_size = 0;
            return read_count;
        }

        // ok
        else {
            d->buf_size = read_count;
        }
    }

    struct sys_dir_ent *dent = (void *)&d->buf[d->buf_idx];
    if (entry) {
        entry->d_ino = dent->fs_id;
        entry->d_off = dent->fs_id;
        entry->d_type = dent->type;
        strncpy(entry->d_name, dent->name, 256);
        entry->d_name[255] = '\0';
    }
    d->last_fs_id = dent->fs_id;
    d->buf_idx += dent->size;

    return 1;
}

int readdir_r(DIR *d, struct dirent *entry, struct dirent **result)
{
    int rc = readdir_safe(d, entry);
    if (rc < 0) {
        return rc;
    } else if (rc > 0) {
        if (result) *result = entry;
        return 0;
    }

    if (result) *result = NULL;
    return 0;
}

struct dirent *readdir(DIR *d)
{
    int rc = readdir_safe(d, &d->dirent);
    return rc > 0 ? &d->dirent : NULL;
}


/*
 * Create
 */
int mkdir(const char *pathname, unsigned int mode)
{
    size_t len = strlen(pathname);
    const char *last = pathname + len - 1;

    // get rid of trailing '/'s
    while (last && last > pathname && *last == '/') {
        last--;
    }

    // pathname doesn't contain any valid path
    if (!last || last <= pathname) {
        return -1;
    }

    const char *new_name = last;
    while (new_name && new_name > pathname && *new_name != '/') {
        new_name--;
    }

    // invalid name
    if (!new_name) {
        return -1;
    }

    // skip '/'
    if (*new_name == '/') {
        new_name++;
    }

    // duplicate base path
    size_t base_len = new_name - pathname;
    char *base = malloc(base_len + 1);
    if (!base) {
        return -1;
    }
    memcpy(base, pathname, base_len);
    base[base_len] = '\0';

    // acquire base path
    int fd = sys_api_acquire(base);
    if (fd < 0) {
        free(base);
        return -1;
    }

    // duplicate name
    size_t name_len = last - new_name + 1;
    char *name = malloc(name_len + 1);
    if (!name) {
        free(base);
        return -1;
    }
    memcpy(name, new_name, name_len);
    name[name_len] = '\0';

    // create new dir
    int err = sys_api_dir_create(fd, name, mode);
    free(base);
    free(name);

    return err;
}


/*
 * Remove
 */
int rmdir(const char *pathname)
{
    int fd = sys_api_acquire(pathname);
    if (fd < 0) {
        return -1;
    }

    int err = sys_api_dir_remove(fd);
    return err;
}
