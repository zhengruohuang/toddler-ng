#ifndef __HAL_INCLUDE_LIB_H__
#define __HAL_INCLUDE_LIB_H__


#include "libk/include/bit.h"
#include "libk/include/debug.h"
#include "libk/include/math.h"
#include "libk/include/string.h"
#include "libk/include/page.h"
#include "libk/include/stack.h"


extern void mempool_free(void *ptr);
extern void *mempool_alloc(size_t size);


#endif
