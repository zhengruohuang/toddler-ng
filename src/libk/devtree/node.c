#include "common/include/inttypes.h"
#include "common/include/devtree.h"
#include "libk/include/devtree.h"
#include "libk/include/debug.h"
#include "libk/include/string.h"
#include "libk/include/bit.h"


/*
 * Find node/prop
 */
struct devtree_node *devtree_find_node_by_phandle(struct devtree_node *node,
                                                  int phandle)
{
    if (phandle <= 0) {
        return NULL;
    }

    if (!node) {
        node = devtree_get_root();
    }

    if (phandle != -1 && devtree_get_phandle(node) == phandle) {
        return node;
    }

    struct devtree_node *child = devtree_get_child_node(node);
    while (child) {
        struct devtree_node *n = devtree_find_node_by_phandle(child, phandle);
        if (n) {
            return n;
        }

        child = devtree_get_next_node(child);
    }

    return NULL;
}

struct devtree_node *devtree_find_child_node(struct devtree_node *node,
                                             const char *name)
{
    if (!node) {
        return devtree_get_root();
    }

    // First pass: exact match
    for (struct devtree_node *child = devtree_get_child_node(node);
         child; child = devtree_get_next_node(child)
    ) {
        const char *child_name = devtree_get_node_name(child);
        if (!strcmp(child_name, name)) {
            return child;
        }
    }

    // Second pass: partial match before @
    // Thus the arg ``name'' must not contain @
    if (!strchr(name, '@')) {
        int name_len = strlen(name);

        for (struct devtree_node *child = devtree_get_child_node(node);
             child; child = devtree_get_next_node(child)
        ) {
            const char *child_name = devtree_get_node_name(child);
            const char *child_name_at = strchr(child_name, '@');
            if (child_name_at) {
                int to_at_len = child_name_at - child_name;
                if (to_at_len == name_len &&
                    !memcmp(name, child_name, to_at_len)
                ) {
                    return child;
                }
            }
        }
    }

    return NULL;
}


struct devtree_prop *devtree_find_prop(struct devtree_node *node,
                                       const char *name)
{
    if (!node) {
        return NULL;
    }

    for (struct devtree_prop *prop = devtree_get_prop(node);
        prop; prop = devtree_get_next_prop(prop)
    ) {
        const char *prop_name = devtree_get_prop_name(prop);

        // Exact match
        if (!strcmp(prop_name, name)) {
            return prop;
        }

        // Partial match before @
        else if (!strchr(name, '@')) {
            const char *prop_name_at = strchr(prop_name, '@');
            if (prop_name_at) {
                int to_at_len = prop_name_at - prop_name;
                if (to_at_len == strlen(name) &&
                    !memcmp(name, prop_name, to_at_len)
                ) {
                    return prop;
                }
            }
        }
    }

    return NULL;
}


/*
 * Walk
 */
struct devtree_node *devtree_walk(const char *path)
{
    if (!path || *path != '/') {
        return NULL;
    }

    char name[64];
    const char *next_path = path;
    struct devtree_node *cur = NULL;

    while (next_path && path && *path && *path != ':') {
        next_path = strpbrk(path, "/:");
        if (next_path) {
            size_t len = (ulong)next_path - (ulong)path;
            if (len) {
                if (!cur) {
                    return NULL;
                }

                memcpy(name, path, len);
                name[len] = '\0';
            }
        } else {
            strcpy(name, path);
        }

        cur = devtree_find_child_node(cur, cur ? name : "root");
        path = next_path + 1;

        if (!cur) {
            return NULL;
        }
    }

    return cur;
}
