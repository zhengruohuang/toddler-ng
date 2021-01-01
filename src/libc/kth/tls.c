#include "common/include/inttypes.h"
#include "libk/include/mem.h"

#include <sys.h>
#include <assert.h>
#include <atomic.h>


static ulong cur_tls_offset = 0;


static void init_tls_var(ulong *offset, size_t size)
{
    ulong aligned_size = align_up_vsize(size, sizeof(ulong));

    ulong old_offset, new_offset;
    do {
        atomic_mb();
        old_offset = *offset;
        if (old_offset != -1ul) {
            return;
        }
        new_offset = old_offset + aligned_size;
    } while (!atomic_cas(offset, old_offset, new_offset));

    ulong tls_size = syscall_get_tib()->tls_size;
    panic_if(new_offset < tls_size, "TLS out of space!\n");
}

void *access_tls_var(ulong *offset, size_t size)
{
    if (*offset == -1) {
        init_tls_var(offset, size);
    }

    ulong tls_base = (ulong)syscall_get_tib()->tls;
    return (void *)(tls_base + *offset);
}
