#ifndef __LIBK_INCLUDE_DEVTREE_H__
#define __LIBK_INCLUDE_DEVTREE_H__


#include "common/include/inttypes.h"
#include "common/include/devtree.h"


/*
 * Node
 */
extern struct devtree_node *devtree_find_node_by_phandle(struct devtree_node *node,
                                                         int phandle);
extern struct devtree_node *devtree_find_child_node(struct devtree_node *node,
                                                    const char *name);
extern struct devtree_prop *devtree_find_prop(struct devtree_node *node,
                                              const char *name);
extern int devtree_in_subtree(struct devtree_node *parent, struct devtree_node *node);
extern struct devtree_node *devtree_walk(const char *path);


/*
 * Prop
 */
extern void devtree_get_reg_num_cells(struct devtree_node *node,
    int *num_addr_cells, int *num_size_cells);
extern void devtree_get_ranges_num_cells(struct devtree_node *node,
    int *num_child_addr_cells, int *num_parent_addr_cells, int *num_size_cells);

extern u64 devtree_translate_addr(struct devtree_node *node, u64 addr);
extern int devtree_get_addr(struct devtree_node *node, int idx,
    const char *name, u64 *addr, u64 *size);
extern int devtree_get_reg(struct devtree_node *node, int idx, u64 *addr,
    u64 *size);
extern int devtree_get_translated_addr(struct devtree_node *node, int idx,
    const char *name, u64 *addr, u64 *size);
extern int devtree_get_translated_reg(struct devtree_node *node, int idx,
    u64 *addr, u64 *size);

extern int devtree_get_reg_shift(struct devtree_node *node);

extern int devtree_get_compatible(struct devtree_node *node, int idx,
    char **name);

extern int devtree_get_phandle(struct devtree_node *node);

extern u64 devtree_get_clock_frequency(struct devtree_node *node);

extern int devtree_is_intc(struct devtree_node *node);
extern int devtree_get_num_int_cells(struct devtree_node *node);
extern int devtree_get_int_parent(struct devtree_node *node);
extern int *devtree_get_int_encode(struct devtree_node *node, int *len);
extern int devtree_get_int_ext(struct devtree_node *node, int pos,
                               int *parent_phandle, void **parent_node,
                               void **encode, int *encode_len);
extern int devtree_get_int(struct devtree_node *node, int pos,
                           int *parent_phandle, void **parent_node,
                           void **encode, int *encode_len);

extern int devtree_get_use_ioport(struct devtree_node *node);
extern int devtree_get_use_poll(struct devtree_node *node);
extern int devtree_get_enabled(struct devtree_node *node);


/*
 * Print
 */
extern void devtree_print(struct devtree_node *node);


/*
 * General
 */
extern struct devtree_node *devtree_alloc_node(struct devtree_node *parent,
    const char *name);
extern struct devtree_prop *devtree_alloc_prop(struct devtree_node *node,
    const char *name, const void *data, int len);
extern struct devtree_prop *devtree_alloc_prop_u32(struct devtree_node *node,
    const char *name, u32 data);
extern struct devtree_prop *devtree_alloc_prop_u64(struct devtree_node *node,
    const char *name, u64 data);
extern struct devtree_prop *devtree_alloc_prop_str(struct devtree_node *node,
    const char *name, char *str);

extern struct devtree_head *devtree_get_head();
extern struct devtree_node *devtree_get_root();
extern struct devtree_node *devtree_get_next_node(struct devtree_node *node);
extern struct devtree_node *devtree_get_child_node(struct devtree_node *node);
extern struct devtree_node *devtree_get_parent_node(struct devtree_node *node);
extern struct devtree_prop *devtree_get_prop(struct devtree_node *node);
extern struct devtree_prop *devtree_get_next_prop(struct devtree_prop *prop);

extern void *devtree_get_data(int offset);
extern char *devtree_get_node_name(struct devtree_node *node);
extern char *devtree_get_prop_name(struct devtree_prop *prop);
extern void *devtree_get_prop_data(struct devtree_prop *prop);
extern u32 devtree_get_prop_data_u32(struct devtree_prop *prop);
extern u64 devtree_get_prop_data_u64(struct devtree_prop *prop);

extern void open_libk_devtree(struct devtree_head *devtree);
extern struct devtree_head *create_libk_devtree(void *_buf, ulong size);

extern size_t devtree_get_buf_size();
extern size_t devtree_get_buf_size2(struct devtree_head *dt);


#endif
