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
    size_t buf_size;
};

struct devtree_node {
    int name;
    int prop;
    int parent;
    int child;
    int next;
};

struct devtree_prop {
    int name;
    int data;
    int len;
    int parent;
    int next;
};


#define DEVTREE_DATA(head, offset) (offset ? (void *)head + offset : NULL)
#define DEVTREE_OFFSET(head, ptr)  (ptr ? (int)((void *)ptr - (void *)head) : 0)


#endif
