#ifndef __LIBK_INCLUDE_COREIMG_H__
#define __LIBK_INCLUDE_COREIMG_H__


#include "common/include/inttypes.h"


extern void init_libk_coreimg(void *img);
extern int is_coreimg_header(void *img);
extern void *get_coreimg();
extern ulong coreimg_size();

extern void *coreimg_find_file(const char *name);
extern int coreimg_find_file_idx(const char *name);
extern int coreimg_file_count();
extern int coreimg_get_filename(int idx, char *buf, int buf_size);
extern void *coreimg_get_file(int idx);
extern void *coreimg_get_file2(int idx, size_t *size);


#endif
