#include <stdio.h>
#include <stdlib.h>
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

static void puts_dev_serial()
{
    //const char *text = "From serial!\n";

    char digits[4] = { '0', '0', '0', '0' };
    char *buf = malloc(5121);
    char *num = buf;
    for (int i = 0; i < 1024; i++) {
        char *s = num;
        s[0] = digits[0];
        s[1] = digits[1];
        s[2] = digits[2];
        s[3] = digits[3];
        s[4] = '\n';
        num += 5;

        if (digits[3] == '9') {
            digits[3] = '0';
            if (digits[2] == '9') {
                digits[2] = '0';
                if (digits[1] == '9') {
                    digits[1] = '0';
                    digits[0]++;
                } else {
                    digits[1]++;
                }
            } else {
                digits[2]++;
            }
        } else {
            digits[3]++;
            //kprintf("digits: %s\n", digits);
        }
    }
    buf[5120] = '\0';

    FILE *f = fopen("/dev/serial", "w");
    fwrite(buf, 1, 5121, f);
    fclose(f);

    free(buf);
}


/*
 * Entry
 */
void test_dev()
{
    cat_dev_zero();
    eof_dev_null();
    //cat_dev_serial();
    //puts_dev_serial();
}
