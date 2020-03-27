#ifndef __LOADER_INCLUDE_DEVTREE_H__
#define __LOADER_INCLUDE_DEVTREE_H__


#include "libk/include/devtree.h"


// #include "common/include/inttypes.h"
// #include "common/include/devtree.h"
//
//
// /*
//  * Node
//  */
// extern struct devtree_node *devtree_find_child_node(struct devtree_node *node,
//     const char *name);
// extern struct devtree_prop *devtree_find_prop(struct devtree_node *node,
//     const char *name);
// extern struct devtree_node *devtree_walk(const char *path);
//
//
// /*
//  * Prop
//  */
// extern void devtree_get_reg_num_cells(struct devtree_node *node,
//     int *num_addr_cells, int *num_size_cells);
// extern void devtree_get_ranges_num_cells(struct devtree_node *node,
//     int *num_child_addr_cells, int *num_parent_addr_cells, int *num_size_cells);
//
// extern u64 devtree_translate_addr(struct devtree_node *node, u64 addr);
// extern int devtree_get_addr(struct devtree_node *node, int idx,
//     const char *name, u64 *addr, u64 *size);
// extern int devtree_get_reg(struct devtree_node *node, int idx, u64 *addr,
//     u64 *size);
// extern int devtree_get_translated_addr(struct devtree_node *node, int idx,
//     const char *name, u64 *addr, u64 *size);
// extern int devtree_get_translated_reg(struct devtree_node *node, int idx,
//     u64 *addr, u64 *size);
// extern int devtree_get_compatible(struct devtree_node *node, int idx,
//     char **name);
//
//
// /*
//  * Print
//  */
// extern void devtree_print(struct devtree_node *node);
//
//
// /*
//  * General
//  */
// extern struct devtree_node *devtree_alloc_node(struct devtree_node *parent,
//     const char *name);
// extern struct devtree_prop *devtree_alloc_prop(struct devtree_node *node,
//     const char *name, const void *data, int len);
// extern struct devtree_prop *devtree_alloc_prop_u32(struct devtree_node *node,
//     const char *name, u32 data);
// extern struct devtree_prop *devtree_alloc_prop_u64(struct devtree_node *node,
//     const char *name, u64 data);
//
// extern struct devtree_head *devtree_get_head();
// extern struct devtree_node *devtree_get_root();
// extern struct devtree_node *devtree_get_next_node(struct devtree_node *node);
// extern struct devtree_node *devtree_get_child_node(struct devtree_node *node);
// extern struct devtree_node *devtree_get_parent_node(struct devtree_node *node);
// extern struct devtree_prop *devtree_get_prop(struct devtree_node *node);
// extern struct devtree_prop *devtree_get_next_prop(struct devtree_prop *prop);
//
// extern void *devtree_get_data(int offset);
// extern char *devtree_get_node_name(struct devtree_node *node);
// extern char *devtree_get_prop_name(struct devtree_prop *prop);
// extern void *devtree_get_prop_data(struct devtree_prop *prop);
// extern u32 devtree_get_prop_data_u32(struct devtree_prop *prop);
// extern u64 devtree_get_prop_data_u64(struct devtree_prop *prop);
//
// extern void init_devtree();


#endif
