#ifndef __LOADER_INCLUDE_PERIPH_H__
#define __LOADER_INCLUDE_PERIPH_H__


/*
 * General
 */
extern int create_periph(const char *dev_name, const void *data, const char *opts);


/*
 * Periphs
 */
extern int init_ns16550(const void *data, const char *opts);


#endif
