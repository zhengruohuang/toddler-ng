#include "common/include/inttypes.h"
#include "common/include/abi.h"
#include "loader/include/lib.h"
#include "loader/include/mem.h"
#include "loader/include/devtree.h"
#include "loader/include/firmware.h"


static int kargc = 0;
static u32 *kargv;
static u32 *env;

static inline char *get_kargv(int idx)
{
#if (ARCH_WIDTH == 32)
    return (void *)(ulong)kargv[idx];
#elif (ARCH_WIDTH == 64)
    return (void *)(long)(s32)kargv[idx];
#else
#error "Unsupported arch width!"
#endif
}

static inline char *get_env(int idx)
{
#if (ARCH_WIDTH == 32)
    return (void *)(ulong)env[idx];
#elif (ARCH_WIDTH == 64)
    return (void *)(long)(s32)env[idx];
#else
#error "Unsupported arch width!"
#endif
}

static const char *search_line(const char *line, const char *key)
{
    int len = strlen(key);
    const char *cur = strstr(line, key);

    while (cur) {
        int match_at_begin = cur == line || *(cur - 1) == ' ';
        cur += len;

        if (match_at_begin && *cur == '=') {
            return cur + 1;
        }

        cur = strstr(cur, key);
    }

    return NULL;
}

static const char *search_kargv(const char *key)
{
    for (int i = 0; i < kargc; i++) {
        char *line = get_kargv(i);
        const char *val = search_line(line, key);
        if (val) {
            return val;
        }
    }

    return NULL;
}

static const char *search_env(const char *key)
{
    for (int i = 0; get_env(i); i++) {
        char *line = get_env(i);
        if (!strcmp(line, key)) {
            return get_env(i + 1);
        }

        const char *val = search_line(line, key);
        if (val) {
            return val;
        }
    }

    return NULL;
}

static const char *get_val(const char *key)
{
    const char *val = search_kargv(key);
    if (val) {
        return val;
    }

    val = search_env(key);
    return val;
}

static ulong get_ulong(const char *key)
{
    const char *val = get_val(key);
    if (!val) {
        return 0;
    }

    return stoul(val, 0);
}

static int fill_bootargs(char *buf)
{
    int size = 0;

    // Only kargv is copied
    for (int i = 0; i < kargc; i++) {
        char *line = get_kargv(i);
        if (buf) {
            strcpy(buf, line);
        }

        size_t len = strlen(line);
        size += len;
        if (buf) {
            buf += len;
        }

        if (i < kargc - 1) {
            if (buf) {
                buf[0] = ' ';
                buf[1] = '\0';
                buf++;
            }

            size++;
        }
    }

    return size ? size + 1 : 0;
}

static void build_chosen_node()
{
    // Get chosen node
    struct devtree_node *chosen = devtree_walk("/chosen");
    if (!chosen) {
        chosen = devtree_alloc_node(devtree_get_root(), "chosen");
    }

    // Initrd
    u64 initrd_start = (u64)get_ulong("rd_start");
    u64 initrd_size = (u64)get_ulong("rd_size");
    u64 initrd_end = initrd_start + initrd_size;

    if (initrd_start && initrd_size) {
        devtree_alloc_prop_u64(chosen, "initrd-start", initrd_start);
        devtree_alloc_prop_u64(chosen, "initrd-end", initrd_end);
    }

    // Memory size
    u64 memsize = (u64)get_ulong("memsize");
    u64 ememsize = (u64)get_ulong("ememsize");

    if (memsize) {
        devtree_alloc_prop_u64(chosen, "memsize", memsize);
    }

    if (ememsize) {
        devtree_alloc_prop_u64(chosen, "ememsize", ememsize);
    }

    // stdout opts
    const char *modetty0 = get_val("modetty0");
    if (modetty0) {
        devtree_alloc_prop(chosen, "stdout-opts", modetty0,
            strlen(modetty0) + 1);
    }

    // Bootargs
    int bargs_len = fill_bootargs(NULL);
    if (bargs_len) {
        struct devtree_prop *bootargs =
            devtree_alloc_prop(chosen, "bootargs", NULL, bargs_len);
        fill_bootargs(devtree_get_prop_data(bootargs));
    }
}

static void init_karg(void *params)
{
    struct firmware_params_karg *karg = params;

    kargc = karg->kargc;
    kargv = karg->kargv;
    env = karg->env;

    build_chosen_node();
}

DECLARE_FIRMWARE_DRIVER(karg) = {
    .name = "karg",
    .need_fdt = 1,
    .init = init_karg,
};
