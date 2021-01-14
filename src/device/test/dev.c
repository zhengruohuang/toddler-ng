#include <stdio.h>
#include <string.h>
#include <dirent.h>


static void cat_dev_zero()
{
    static char zero_buf[129];
    memset(zero_buf, 1, 129);

    FILE *f = fopen("/dev/zero", "r");
    size_t c = fread(zero_buf, 1, 64, f);
    for (int i = 0; i < 128; i++) {
        zero_buf[i] += '0';
    }
    zero_buf[128] = '\0';
    kprintf("File @ %p, read count: %lu, buf: %s\n", f, c, zero_buf);
    fclose(f);
}

static void eof_dev_null()
{
    FILE *f = fopen("/dev/null", "r");
    size_t c = fread(NULL, 1, 64, f);
    int eof = feof(f);
    kprintf("/dev/null, read count: %lu, eof: %d\n", c, eof);
    fclose(f);
}


/*
 * Entry
 */
void test_dev()
{
    cat_dev_zero();
    eof_dev_null();
}
