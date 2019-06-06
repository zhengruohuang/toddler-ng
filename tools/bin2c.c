#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void start_c(FILE *f, char *symbol, int align, char *section)
{
    if (section) {
        fprintf(f, "unsigned char __attribute__ ((aligned (%d), section (\"%s\"))) %s[] = {\n",
                align, section, symbol);
    } else {
        fprintf(f, "unsigned char __attribute__ ((aligned (%d))) %s[] = {\n",
                align, symbol);
    }
}

static void finish_c(FILE *f, char *symbol, unsigned long bytes)
{
    fprintf(f, "};\n\n");
    fprintf(f, "unsigned long %s_bytes = 0x%lxul;\n\n", symbol, bytes);
}

static unsigned long copy_to_c(FILE *b, FILE *f)
{
    unsigned char buf[8];
    unsigned long bytes = 0;

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

        bytes += count;
        fprintf(f, "\n");
    }

    return bytes;
}

static int bin2c(char *bin_name, char *c_name, char *symbol, int align, char *section)
{
    FILE *bin_file = fopen(bin_name, "rb");
    if (!bin_file) {
        return -1;
    }

    FILE *c_file = fopen(c_name, "w");
    if (!c_file) {
        return -1;
    }

    start_c(c_file, symbol, align, section);
    unsigned long bytes = copy_to_c(bin_file, c_file);
    finish_c(c_file, symbol, bytes);

    return 0;
}

static void usage()
{
    printf("Usage:\n");
    printf("\tbin2c <bin> <template> <out> <symbol> [align]\n");
}

static int parse_int(int argc, char *argv[], char *name, int def)
{
    for (int i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], name)) {
            return atoi(argv[i + 1]);
        }
    }

    return def;
}

static char *parse_str(int argc, char *argv[], char *name, char *def)
{
    for (int i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], name)) {
            return argv[i + 1];
        }
    }

    return def;
}

int main(int argc, char *argv[])
{
    printf("Toddler binary to C converter\n");

    char *bin_name = parse_str(argc, argv, "--bin", NULL);
    char *c_name = parse_str(argc, argv, "--c", NULL);
    char *symbol = parse_str(argc, argv, "--symbol", NULL);
    if (!bin_name || !c_name || !symbol) {
        usage();
        return -1;
    }

    int align = parse_int(argc, argv, "--align", 8);
    char *section = parse_str(argc, argv, "--section", NULL);

    return bin2c(bin_name, c_name, symbol, align, section);
}
