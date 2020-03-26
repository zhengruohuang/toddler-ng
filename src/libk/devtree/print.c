#include "common/include/inttypes.h"
#include "common/include/arch.h"
#include "common/include/devtree.h"
#include "libk/include/devtree.h"
#include "libk/include/kprintf.h"
#include "libk/include/debug.h"
#include "libk/include/string.h"
#include "libk/include/bit.h"


static void print_ident(int ident)
{
    for (int i = 0; i < ident; i++) {
        __kprintf("  ");
    }
}

// static void print_digit(char c)
// {
//     __kprintf("%c", c > 10 ? 'a' + c - 10 : '0' + c);
// }

static void print_prop_data(struct devtree_prop *prop)
{
    if (!prop->len) {
        __kprintf("(null)");
        return;
    }

    char *data = devtree_get_prop_data(prop);

    int is_str = 1;
    for (int i = 0; i < prop->len; i++) {
        char c = data[i];
        if ((!i && !c) ||
            (c && (c < 32 || c > 126))
        ) {
            is_str = 0;
            break;
        }
    }

    if (is_str) {
        int is_zero = 1;
        for (int i = 0; i < prop->len; i++) {
            char c = data[i];
            if (c) {
                if (is_zero) __kprintf(i ? ", \"" : "\"");
                __kprintf("%c", data[i]);
                is_zero = 0;
            } else {
                if (!is_zero) __kprintf("\"");
                is_zero = 1;
            }
        }
    } else if (!(prop->len & 0x3)) {
        int len32 = prop->len >> 2;
        u32 *data32 = (u32 *)data;
        __kprintf("<");
        for (int i = 0; i < len32; i++) {
            if (i) __kprintf(" ");
            __kprintf("%x", swap_big_endian32(data32[i]));
        }
        __kprintf(">");
    } else {
        for (int i = 0; i < prop->len; i++) {
            __kprintf(i ? " %h" : "[%h", data[i]);
        }
        __kprintf("]");
    }
}

static void print_prop(struct devtree_prop *prop, int indent)
{
    print_ident(indent + 1);
    __kprintf("%s = ", devtree_get_prop_name(prop));
    print_prop_data(prop);
    __kprintf(";\n");
}

static void print_node(struct devtree_node *node, int indent)
{
    print_ident(indent);
    __kprintf("%s {\n", devtree_get_node_name(node));

    struct devtree_prop *prop = devtree_get_prop(node);
    while (prop) {
        print_prop(prop, indent);
        prop = devtree_get_next_prop(prop);
    }

    struct devtree_node *child = devtree_get_child_node(node);
    while (child) {
        print_node(child, indent + 1);
        child = devtree_get_next_node(child);
    }

    print_ident(indent);
    __kprintf("};\n");
}

void devtree_print(struct devtree_node *node)
{
    if (!node) {
        node = devtree_get_root();
        if (!node) {
            return;
        }
    }

    print_node(node, 0);
}
