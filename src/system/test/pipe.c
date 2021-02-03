#include <stdio.h>
#include <string.h>
#include <kth.h>
#include <sys.h>
#include <sys/api.h>


static unsigned long named_pipe_reader(unsigned long param)
{
    int count = param;
    FILE *f = fopen("/pipe", "r");

    char buf[32];
    for (int i = 0; i < count; i++) {
        fread(buf, 1, 32, f);
        kprintf("#%d Read: %s\n", i, buf);
    }

    fclose(f);
    return 0;
}

static unsigned long named_pipe_writer(unsigned long param)
{
    int count = param;
    FILE *f = fopen("/pipe", "w");

    char buf[] = "pipe write!";
    for (int i = 0; i < count; i++) {
        fwrite(buf, 1, strlen(buf) + 1, f);
        fflush(f);
    }

    fclose(f);
    return 0;
}

__unused_func static void test_named_pipe()
{
    const int count = 100;

    kth_t reader_thread, writer_thread;
    kth_create(&reader_thread, named_pipe_reader, count);
    kth_create(&writer_thread, named_pipe_writer, count);

    kth_join(&reader_thread, NULL);
    kth_join(&writer_thread, NULL);
}


/*
 * Entry
 */
void test_pipe()
{
    test_named_pipe();
}
