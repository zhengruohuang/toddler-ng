#include "common/include/inttypes.h"
#include "common/include/coreimg.h"
#include "libk/include/string.h"


static struct coreimg_fat *coreimg = NULL;


void init_libk_coreimg(void *img)
{
    coreimg = img;
}


int is_coreimg_header(void *img)
{
    struct coreimg_fat *hdr = img;
    return hdr->header.magic == COREIMG_MAGIC;
}

void *coreimg_find_file(const char *name)
{
    int file_count = coreimg->header.file_count;
    struct coreimg_record *record = NULL;

    for (int i = 0; i < file_count; i++) {
        record = &coreimg->records[i];
        if (!strncmp(name, record->file_name, 20)) {
            int offset = record->start_offset;
            return (void *)coreimg + offset;
        }
    }

    return NULL;
}


int coreimg_file_count()
{
    int file_count = coreimg->header.file_count;
    return file_count;
}

int coreimg_get_filename(int idx, char *buf, int buf_size)
{
    int file_count = coreimg->header.file_count;
    if (idx >= file_count) {
        return 0;
    }

    struct coreimg_record *record = &coreimg->records[idx];

    int len = 0;
    for (char c = record->file_name[len]; c && len < 20 && len < buf_size - 1;
         c = record->file_name[++len]
    ) {
        buf[len] = c;
    }
    buf[len] = '\0';

    return len;
}

void *coreimg_get_file(int idx)
{
    int file_count = coreimg->header.file_count;
    if (idx >= file_count) {
        return 0;
    }

    struct coreimg_record *record = &coreimg->records[idx];
    int offset = record->start_offset;
    return (void *)coreimg + offset;
}
