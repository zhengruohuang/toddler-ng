#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <sys/api.h>


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

static void cat_dev_serial()
{
    static char serial_buf[33];
    memset(serial_buf, 0, 33);

    FILE *f = fopen("/dev/serial", "r");
    for (int i = 0; i < 2; i++) {
        size_t c = fread(serial_buf, 1, 32, f);
        //ssize_t c = sys_api_file_read(f->fd, serial_buf, 32, 0);
        kprintf("read something!\n");
        serial_buf[c] = '\0';
        kprintf("Serial: %s\n", serial_buf);
    }
    fclose(f);
}


/*
 * Entry
 */
void test_dev()
{
    cat_dev_zero();
    eof_dev_null();
    cat_dev_serial();
}
