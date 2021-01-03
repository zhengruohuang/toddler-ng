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
        if (*offset  -1ul) {
            break;
        }

        *offset = cur_tls_offset;
        cur_tls_offset += align_up_vsize(size, sizeof(ulong));
    }

//     ulong aligned_size = align_up_vsize(size, sizeof(ulong));
//
//     kprintf("init tls @ %p\n", offset);
//
//     ulong old_offset, new_offset;
//     do {
//         atomic_mb();
//         old_offset = *offset;
//         if (old_offset != -1ul) {
//             return;
//         }
//         new_offset = old_offset + aligned_size;
//     } while (!atomic_cas(offset, old_offset, new_offset));
//
//     ulong tls_size = syscall_get_tib()->tls_size;
//     panic_if(new_offset < tls_size,
//              "TLS out of space, old offset: %lu, new: %lu, tls size: %lu\n",
//              old_offset, new_offset, tls_size);
}

void *access_tls_var(ulong *offset, size_t size)
{
    if (*offset == -1ul) {
        init_tls_var(offset, size);
    }

    ulong tls_base = (ulong)syscall_get_tib()->tls;
    return (void *)(tls_base + *offset);
}
