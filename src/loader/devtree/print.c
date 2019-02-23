#include "common/include/inttypes.h"
#include "common/include/arch.h"
#include "common/include/devtree.h"
#include "loader/include/devtree.h"
#include "loader/include/lib.h"
#include "loader/include/lprintf.h"


static void print_ident(int ident)
{
    for (int i = 0; i < ident; i++) {
        lprintf("  ");
    }
}

static void print_digit(char c)
{
    lprintf("%c", c > 10 ? 'a' + c - 10 : '0' + c);
}

static void print_prop_data(struct devtree_prop *prop)
{
    if (!prop->len) {
        lprintf("(null)");
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
                if (is_zero) lprintf(i ? ", \"" : "\"");
                lprintf("%c", data[i]);
                is_zero = 0;
            } else {
                if (!is_zero) lprintf("\"");
                is_zero = 1;
            }
        }
    } else if (!(prop->len & 0x3)) {
        int len32 = prop->len >> 2;
        u32 *data32 = (u32 *)data;
        lprintf("<");
        for (int i = 0; i < len32; i++) {
            if (i) lprintf(" ");
            lprintf("%x", swap_big_endian(data32[i]));
        }
        lprintf(">");
    } else {
        for (int i = 0; i < prop->len; i++) {
            lprintf(i ? " %h" : "[%h", data[i]);
        }
        lprintf("]");
    }
}

static void print_prop(struct devtree_prop *prop, int indent)
{
    print_ident(indent + 1);
    lprintf("%s = ", devtree_get_prop_name(prop));
    print_prop_data(prop);
    lprintf(";\n");
}

static void print_node(struct devtree_node *node, int indent)
{
    print_ident(indent);
    lprintf("%s {\n", devtree_get_node_name(node));

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
    lprintf("};\n");
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
