#ifndef __LIBC_INCLUDE_STDIO_H__
#define __LIBC_INCLUDE_STDIO_H__


/*
 * kprintf
 */
#include "libk/include/kprintf.h"

extern void init_kprintf();
extern int kprintf_unlocked(const char *fmt, ...);


/*
 * File
 */
// size_t and ssize_t
#include "common/include/inttypes.h"

#define BUFSIZE         8192
#define EOF             (-1)
#define FILENAME_MAX    256
#define FOPEN_MAX       32
#define L_tmpnam        0
#define TMP_MAX         32

enum seek_whence {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
};

typedef ssize_t fpos_t;

typedef struct __FILE {
    int fd;
    unsigned long fs_id;
    unsigned long pos;

    char buf[BUFSIZE];
    int buf_size;
    int buf_idx;
    char buf_dirty;

    char dyn_alloc;
    char eof;
} FILE;


extern FILE *fopen(const char *pathname, const char *mode);
extern int fclose(FILE *f);
extern FILE *freopen(const char *pathname, const char *mode, FILE *f);

extern long ftell(FILE *f);
extern int fseek(FILE *f, long offset, int whence);
extern int fgetpos(FILE *f, fpos_t *pos);
extern int fsetpos(FILE *f, const fpos_t *pos);
extern void rewind(FILE *f);
extern int feof(FILE *f);

extern void setbuf(FILE *f, char *buf);
extern int setvbuf(FILE *f, char *buf, int mode, size_t size);

int ferror(FILE *f);
void clearerr(FILE *f);

extern size_t fread(void *ptr, size_t size, size_t count, FILE *f);
extern size_t fwrite(const void *ptr, size_t size, size_t count, FILE *f);
extern int fflush(FILE *f);

extern int fgetc(FILE *f);
extern int fputc(int c, FILE *f);
extern char *fgets(char *s, int n, FILE *f);
extern int fputs(const char *s, FILE *f);
static inline int getc(FILE *f) { return fgetc(f); }
static inline int putc(int c, FILE *f) { return fputc(c, f); }
extern int ungetc(int c, FILE *f);


/*
 * Stdio
 */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

static inline int getchar() { return getc(stdin); }
static inline int putchar(int c) { return putc(c, stdout); }
static inline int ungetchar(int c) { return ungetc(c, stdin); }


/*
 * Error
 */
extern void perror(const char *str);


// operations
// remove, rename, tmpfile, tmpnam


/*
 * Formatted IO
 */
extern int printf(const char *fmt, ...);
extern int fprintf(FILE *f, const char *fmt, ...);
extern int vprintf(const char *fmt, va_list arg);
extern int vfprintf(FILE *f, const char *fmt, va_list arg);

// scanf, fscanf
// snprintf, sprintf, vsnprintf, vsprintf
// sscanf, vfprintf, vscanf, vsscanf


#endif
