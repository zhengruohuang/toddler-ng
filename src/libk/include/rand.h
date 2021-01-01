#ifndef __LIBK_INCLUDE_RAND_H__
#define __LIBK_INCLUDE_RAND_H__


#include "common/include/inttypes.h"


/*
 * Rand
 */
extern int rand();
extern int rand_r(unsigned int *seedp);
extern void srand(unsigned int seed);


#endif
