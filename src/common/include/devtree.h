#ifndef __COMMON_INCLUDE_DEVTREE_H__
#define __COMMON_INCLUDE_DEVTREE_H__


#include "common/include/compiler.h"
#include "common/include/inttypes.h"


/*
 * All in offset
 */
struct devtree_head {
    int root;
    int strs;
} packed_struct;

struct devtree_node {
    int name;
    int prop;
    int parent;
    int child;
    int next;
} packed_struct;

struct devtree_prop {
    int name;
    int data;
    int len;
    int parent;
    int next;
} packed_struct;


#define DEVTREE_DATA(head, offset) (offset ? (void *)head + offset : NULL)
#define DEVTREE_OFFSET(head, ptr)  (ptr ? (int)((void *)ptr - (void *)head) : 0)


#endif
