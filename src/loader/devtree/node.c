#include "common/include/inttypes.h"
#include "common/include/devtree.h"
#include "loader/include/lib.h"
#include "loader/include/devtree.h"


/*
 * Find node/prop
 */
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
        if (!strcmp(prop_name, name)) {
            return prop;
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
