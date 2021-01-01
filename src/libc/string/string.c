#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/*
 * String
 */
char *strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (!d) {
        return NULL;
    }

    memcpy(d, s, len);
    return d;
}

// appends the string src to dest
char *strcat(char *dest, const char *src)
{
    return NULL;
}

// appends at most n bytes of the string src to dest
char *strncat(char *dest, const char *src, size_t n)
{
    return NULL;
}

// locates byte c in a string, searching from the end
char *strrchr(const char *str, int c)
{
    return NULL;
}

// compares two strings using the current locale's collating order
int strcoll(const char *str1, const char *str2)
{
    return 0;
}

// determines the length of the maximal initial substring consisting entirely of bytes in accept
size_t strspn(const char *str, const char *accept)
{
    return 0;
}

// determines the length of the maximal initial substring consisting entirely of bytes not in reject
size_t strcspn(const char *str, const char *reject)
{
    return 0;
}

// parses a string into a sequence of tokens; non-thread safe in the spec, non-reentrant[1]
char *strtok(char *str, const char *delim)
{
    return NULL;
}

// transforms src into a collating form, such that the numerical sort order of the transformed string is equivalent to the collating order of src
size_t strxfrm(char *dest, const char *src, size_t n)
{
    return 0;
}

// bounds-checked variant of strcat
// ISO/IEC WDTR 24731
int strcat_s(char *dest, size_t n, const char *src)
{
    return 0;
}

// bounds-checked variant of strcpy
// ISO/IEC WDTR 24731
int strcpy_s(char *dest, size_t n, const char *src)
{
    return 0;
}

// bounds-checked variant of strcat	// originally OpenBSD, now also FreeBSD, Solaris, Mac OS X
size_t strlcat(char *dest, const char *src, size_t n)
{
    return 0;
}

// bounds-checked variant of strcpy
// originally OpenBSD, now also FreeBSD, Solaris, Mac OS X
size_t strlcpy(char *dest, const char *src, size_t n)
{
    return 0;
}

// thread-safe and reentrant version of strtok
// POSIX
char *strtok_r(char *str, const char *delim, char **saveptr)
{
    return NULL;
}


/*
 * Error
 */
// returns the string representation of an error number e.g. errno (not thread-safe)
char *strerror(int errno)
{
    return NULL;
}

// Puts the result of strerror() into the provided buffer in a thread-safe way
// POSIX:2001
int strerror_r_posix(int errno, char *buf, size_t n)
{
    return 0;
}

// Return strerror() in a thread-safe way. The provided buffer is used only if necessary
// GNU, incompatible with POSIX version
char *strerror_r_gnu(int errno, char *buf, size_t n)
{
    return NULL;
}

// by analogy to strerror, returns string representation of the signal sig (not thread safe)
// POSIX:2008[3]
char *strsignal(int sig)
{
    return NULL;
}


/*
 * Mem
 */
// copies n bytes between two memory areas; unlike with memcpy the areas may overlap
void *memmove(void *dest, const void *src, size_t n)
{
    return NULL;
}

// copies up to n bytes between two memory areas, which must not overlap, stopping when the byte c is found
// SVID, POSIX[2]
void *memccpy(void *dest, const void *src, int c, size_t n)
{
    return NULL;
}

// variant of memcpy returning a pointer to the byte following the last written byte
// GNU
void *mempcpy(void *dest, const void *src, size_t n)
{
    return NULL;
}
