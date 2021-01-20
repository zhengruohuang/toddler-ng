#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/api.h>
#include "libk/include/math.h"


/*
 * Helpers
 */
static salloc_obj_t file_salloc = SALLOC_CREATE_DEFAULT(sizeof(FILE));

static inline void reset_fbuf(FILE *f)
{
    f->buf_idx = f->buf_size = 0;
    f->buf_dirty = 0;
}


/*
 * Open and close
 */
FILE *fopen(const char *pathname, const char *mode)
{
    int fd = sys_api_acquire(pathname);
    if (fd < 0) {
        return NULL;
    }

    int err = sys_api_file_open(fd, 0, 0);
    if (err) {
        return NULL;
    }

    FILE *f = salloc(&file_salloc);
    if (!f) {
        return NULL;
    }

    f->fd = fd;
    f->dyn_alloc = 1;
    return f;
}

int fclose(FILE *f)
{
    if (!f) {
        return -1;
    }

    int err = fflush(f);
    if (err) {
        return err;
    }

    err = sys_api_release(f->fd);
    if (err) {
        return err;
    }

    if (f->dyn_alloc) {
        sfree(f);
    } else {
        memzero(f, sizeof(FILE));
    }
    return 0;
}

FILE *freopen(const char *pathname, const char *mode, FILE *f)
{
    if (!f) {
        return NULL;
    }

    if (pathname) {
        fclose(f);
        return fopen(pathname, mode);
    }

    reset_fbuf(f);
    f->pos = 0;
    f->eof = 0;

    int err = sys_api_file_open(f->fd, 0, 0);
    if (err) {
        return NULL;
    }

    return f;
}


/*
 * Position
 */
long ftell(FILE *f)
{
    return f->pos + f->buf_idx;
}

int fseek(FILE *f, long offset, int whence)
{
    if (!f) {
        return -1;
    }

    int err = fflush(f);
    if (err) {
        return err;
    }

    long pos = 0;
    switch (whence) {
    case SEEK_SET:
        pos = offset;
        break;
    case SEEK_CUR:
        pos = (long)f->pos + offset;
        break;
    case SEEK_END:
        panic("Not implemented!\n");
        return -1;
    default:
        return -1;
    }

    if (pos < 0) {
        return -1;
    }

    f->pos = pos;
    return 0;
}

int fgetpos(FILE *f, fpos_t *pos)
{
    if (!f) {
        return -1;
    }

    if (pos) *pos = f->pos + f->buf_idx;
    return 0;
}

int fsetpos(FILE *f, const fpos_t *pos)
{
    if (!f || !pos) {
        return -1;
    }

    int err = fflush(f);
    if (err) {
        return err;
    }

    f->pos = *pos;
    return 0;
}

void rewind(FILE *f)
{
    fflush(f);

    f->pos = 0;
    f->eof = 0;
}

int feof(FILE *f)
{
    return f->eof;
}


/*
 * Buffer
 */
void setbuf(FILE *f, char *buf)
{
    panic("Not implemented!\n");
}

int setvbuf(FILE *f, char *buf, int mode, size_t size)
{
    panic("Not implemented!\n");
    return -1;
}


/*
 * Error
 */
int ferror(FILE *f)
{
    panic("Not implemented!\n");
    return 0;
}

void clearerr(FILE *f)
{
    panic("Not implemented!\n");
}


/*
 * Read and write
 */
static inline size_t _read_fbuf(void *ptr, size_t bytes, FILE *f)
{
    // Buf is empty
    if (f->buf_idx >= f->buf_size) {
        panic_if(f->buf_size, "fbuf in unknown state!\n");
        return 0;
    }

    size_t buf_bytes = f->buf_size - f->buf_idx;

    // There's enough data in file buf
    if (buf_bytes >= bytes) {
        memcpy(ptr, &f->buf[f->buf_idx], bytes);
        f->buf_idx += bytes;

        // Reset buf idx if needed
        if (f->buf_idx == f->buf_size) {
            f->buf_idx = f->buf_size = 0;
            f->pos += buf_bytes;
        }

        // Done
        return bytes;
    }

    // No enough data in file buf
    // Copy everything from fbuf and reset it
    memcpy(ptr, &f->buf[f->buf_idx], buf_bytes);
    f->buf_idx = f->buf_size = 0;
    f->pos += buf_bytes;

    return buf_bytes;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *f)
{
    if (!f || !ptr || !size || !count) {
        return 0;
    }

    if (f->eof) {
        return 0;
    }

    if (f->buf_dirty) {
        int err = fflush(f);
        if (err) {
            return err;
        }
    }

    // TODO: replace with mul func
    size_t total_bytes = size * count;
    size_t read_bytes = _read_fbuf(ptr, total_bytes, f);
    panic_if(read_bytes > total_bytes, "Read more than requested!\n");

    // Already got enough data
    if (read_bytes == total_bytes) {
        return count;
    }

    // Update counters
    ptr += read_bytes;
    total_bytes -= read_bytes;

    // Read as much as possible
    ssize_t rc = sys_api_file_read(f->fd, ptr, total_bytes, f->pos);
    if (rc < 0) {
        // TODO: set err code
        //return 0;
    } else if (!rc) {
        f->eof = 1;
    } else {
        panic_if(rc > total_bytes, "VFS returned more than requested!\n");

        ptr += rc;
        read_bytes += rc;
        total_bytes -= rc;
        f->pos += rc;
    }

    // User buffer is filled
    if (!total_bytes) {
        return count;
    }

    // Find read count
    size_t read_count = 0, tail_bytes = 0;
    div_ulong(read_bytes, size, &read_count, &tail_bytes);
    panic_if(tail_bytes > BUFSIZE, "Tail size exceeding BUFSIZE!\n");

    // // Put tail bytes back to internal buf
    //memcpy(f->buf, ptr + (read_bytes - tail_bytes), tail_bytes);
    //f->buf_size = tail_bytes;
    f->pos -= tail_bytes;

    return read_count;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *f)
{
    if (!f || !ptr || !size || !count) {
        return 0;
    }

    // TODO: replace with mul func
    size_t total_bytes = size * count;

    // Write to buffer first
    panic_if(f->buf_size < f->buf_idx, "Inconsistent file buf size!\n");
    size_t buf_write_bytes = f->buf_size - f->buf_idx;
    if (buf_write_bytes > total_bytes) {
        buf_write_bytes = total_bytes;
    }
    memcpy(f->buf + f->buf_idx, ptr, buf_write_bytes);
    f->buf_dirty = 1;

    if (f->buf_size == f->buf_idx) {
        fflush(f);
    }

    // Adjust size, done if all written to the buf
    size_t write_bytes = buf_write_bytes;
    ptr += buf_write_bytes;
    total_bytes -= buf_write_bytes;
    if (!total_bytes) {
        return write_bytes;
    }

    // Write as much as possible
    ssize_t wc = sys_api_file_write(f->fd, ptr, total_bytes, f->pos);
    if (wc < 0) {
        // TODO: set err code
        //return 0;
    } else if (!wc) {
        //f->eof = 1;
    } else {
        panic_if(wc > total_bytes, "VFS wrote more than supplied!\n");

        ptr += wc;
        write_bytes += wc;
        total_bytes -= wc;
        f->pos += wc;
    }

    // Done
    return write_bytes;
}

int fflush(FILE *f)
{
    if (!f) {
        return -1;
    }

    if (f->buf_dirty) {
        ssize_t wc = sys_api_file_write(f->fd, f->buf, f->buf_idx, f->pos);
        if (wc < 0) {
            // TODO: set err code
            //return 0;
            return -1;
        } else {
            panic_if(wc > f->buf_idx, "VFS wrote more than supplied!\n");
            f->pos += wc;
        }
    }

    reset_fbuf(f);
    return 0;
}


/*
 * Char
 */
int fgetc(FILE *f)
{
    return -1;
}

int fputc(int c, FILE *f)
{
    return -1;
}

char *fgets(char *s, int n, FILE *f)
{
    return NULL;
}

int fputs(const char *s, FILE *f)
{
    return -1;
}

int ungetc(int c, FILE *f)
{
    return -1;
}
