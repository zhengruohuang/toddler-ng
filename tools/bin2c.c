#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void start_c(FILE *f, char *symbol, int align)
{
    fprintf(f, "unsigned char __attribute__ ((aligned (%d))) %s[] = {\n", align, symbol);
}

static void finish_c(FILE *f, char *symbol, int align)
{
    fprintf(f, "};\n\n");
}

static void copy_to_c(FILE *b, FILE *f)
{
    unsigned char buf[8];

    for (int count = fread(buf, 1, sizeof(buf), b); count;
        count = fread(buf, 1, sizeof(buf), b)
    ) {
        fprintf(f, "    ");
        for (int i = 0; i < count; i++) {
            unsigned char byte = buf[i];
            unsigned char high = byte >> 4;
            unsigned char low = byte & 0xf;
            fprintf(f, "0x%c%c, ",
                high >= 10 ? 'a' + (high - 10) : '0' + high,
                low >= 10  ? 'a' + (low - 10)  : '0' + low);
        }

        fprintf(f, "\n");
    }
}

static int bin2c(char *bin_name, char *c_name, char *symbol, int align)
{
    FILE *bin_file = fopen(bin_name, "rb");
    if (!bin_file) {
        return -1;
    }

    FILE *c_file = fopen(c_name, "w");
    if (!c_file) {
        return -1;
    }

    start_c(c_file, symbol, align);
    copy_to_c(bin_file, c_file);
    finish_c(c_file, symbol, align);

    return 0;
}

static void usage()
{
    printf("Usage:\n");
    printf("\tbin2c <bin> <template> <out> <symbol> [align]\n");
}

int main(int argc, char *argv[])
{
    printf("Toddler binary to C converter\n");

    // Check arguments
    if (argc < 4) {
        usage();
        return -1;
    }

    return bin2c(argv[1], argv[2], argv[3],
        argc >= 5 ? atoi(argv[4]) : 8
    );
}
