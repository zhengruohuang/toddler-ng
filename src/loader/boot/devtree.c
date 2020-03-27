#include "common/include/inttypes.h"
#include "loader/include/devtree.h"


#define DEVTREE_BUF_SIZE 128 * 1024

static char buf[DEVTREE_BUF_SIZE];


void init_devtree()
{
    create_libk_devtree(buf, DEVTREE_BUF_SIZE);
}

