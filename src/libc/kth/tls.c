#include "common/include/inttypes.h"
#include "libk/include/mem.h"

#include <sys.h>
#include <assert.h>
#include <atomic.h>
#include <kth.h>


static mutex_t tls_mutex = MUTEX_INITIALIZER;
static volatile ulong cur_tls_offset = 0;


static void init_tls_var(ulong *offset, size_t size)
{
    mutex_exclusive(&tls_mutex) {
        if (*offset != -1ul) {
            break;
        }

        *offset = cur_tls_offset;
        cur_tls_offset += align_up_vsize(size, sizeof(ulong));
    }
}

void *access_tls_var(ulong *offset, size_t size)
{
    if (*offset == -1ul) {
        init_tls_var(offset, size);
    }

    ulong tls_base = (ulong)syscall_get_tib()->tls;
    return (void *)(tls_base + *offset);
}
