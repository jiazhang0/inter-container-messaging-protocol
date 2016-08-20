/*
 * String tree infrastructure
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef __STRING_TREE_H__
#define __STRING_TREE_H__

#ifdef EEE_IC
  #include <ic.h>
  #define string_tree_set_errno		ic_set_errno
#endif

#ifndef STRING_TREE_ERRNO_BASE
  #error "STRING_TREE_ERRNO_BASE isn't defined"
#endif

struct string_tree_node {
	char *string;
	unsigned int nr_child;
	struct string_tree_node *child;
};

typedef struct string_tree_node	string_tree_node_t;

static inline string_tree_node_t*
__string_tree_get_child_node(string_tree_node_t *parent,
			     unsigned int index)
{
	return parent->child + index;
}

static inline const char *
string_tree_get_node_string(string_tree_node_t *node)
{
	return node->string;
}

extern string_tree_node_t*
string_tree_get_child_node(string_tree_node_t *parent,
			   unsigned int index);

int
string_tree_create_child_node(string_tree_node_t *parent,
			      unsigned int nr_child);

int
string_tree_set_node_string(string_tree_node_t *node,
			    const char *str, unsigned int str_len);

void
string_tree_dump_tree(string_tree_node_t *this_node);

char *
string_tree_query(const char *path);

extern string_tree_node_t
string_tree_root;

#endif	/* __STRING_TREE_H__ */
