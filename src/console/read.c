#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int console_read(char **o_buf, ssize_t *o_buf_size, FILE *f)
{
    size_t buf_size = 1024;
    char *buf = malloc(buf_size + 1);

    size_t len = 0;
    char *cur = buf;

    do {
        fread(cur, 1, 1, f);
        char ch = *cur;

        switch (ch) {
        case 3: // Ctrl+C
            fprintf(f, "^C\n");
            fflush(f);

            buf[0] = '\0';
            len = 1;

            if (o_buf) *o_buf = buf;
            if (o_buf_size) *o_buf_size = len + 1;
            return 0;
        case 4: // Ctrl+D
            free(buf);
            return -1;
        case '\b':
            if (len) {
                len--;
                cur--;
                fprintf(f, "\b \b");
            }
            break;
        case '\r':
            *cur = '\n';
        case '\n':
            fprintf(f, "\n");
            fflush(f);

            cur[1] = '\0';
            len++;

            if (o_buf) *o_buf = buf;
            if (o_buf_size) *o_buf_size = len + 1;
            return 0;
        case '\t':
            ch = ' ';
        default:
            fprintf(f, "%c", ch);
            cur++;
            len++;
            break;
        }

        fflush(f);

        if (len == buf_size) {
            buf_size <<= 1;
            buf = realloc(buf, buf_size);
            cur = buf + len;
        }
    } while (1);

    return -1;
}
