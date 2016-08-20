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

#include "string_tree.h"

string_tree_node_t string_tree_root = {
	.string = ".",
};

string_tree_node_t*
string_tree_get_child_node(string_tree_node_t *parent,
			   unsigned int index)
{
	if (index >= parent->nr_child)
		return NULL;

	return __string_tree_get_child_node(parent, index);
}

int
string_tree_create_child_node(string_tree_node_t *parent,
			      unsigned int nr_child)
{
	string_tree_node_t *child;
	int sz = sizeof(string_tree_node_t) * nr_child;

	child = malloc(sz);
	if (!child)
		return -1;

	memset(child, 0, sz);

	parent->nr_child = nr_child;
	parent->child = child;

	return 0;
}

int
string_tree_set_node_string(string_tree_node_t *node,
			    const char *str, unsigned int str_len)
{
	char *new_str;

	new_str = malloc(str_len + 1);
	if (!new_str)
		return -1;

	strncpy(new_str, str, str_len);
	new_str[str_len] = 0;

	node->string = new_str;

	return 0;
}

static void
print_level(unsigned int level)
{
	for (unsigned int i = 0; i < level; ++i)
		dbg_cont("  ");
}

static void
dump_string_tree(string_tree_node_t *this_node, unsigned int level)
{
	print_level(level);

	dbg_cont("%s\n", this_node->string);

	for (int i = 0; i < this_node->nr_child; ++i)
		dump_string_tree(__string_tree_get_child_node(this_node, i), level + 1);
}

void
string_tree_dump_tree(string_tree_node_t *this_node)
{
	dump_string_tree(this_node, 0);
}

static char *
query_string_tree(string_tree_node_t *this_node, const char *path,
		  int *associated)
{
	dbg("Searching string %s vs %s\n", path, this_node->string);

	int len;
	char *p = strchr(path, '.');
	if (p)
		len = p - path;
	else
		len = strlen(path);

	/* Match up? */
	if (strncmp(this_node->string, path, len) || strlen(this_node->string) != len)
		return NULL;

	*associated = 1;
	path += len;
	if (*path)
		/* Skip "." */
		++path;
	else {
		/* Short of search */
		if (!this_node->nr_child)
			return NULL;

		char *result = NULL;
		unsigned int result_len = 0;
		for (int i = 0; i < this_node->nr_child; ++i) {
			string_tree_node_t *child_node;

			child_node = string_tree_get_child_node(this_node, i);
			if (child_node->nr_child) {
				free(result);
				return NULL;
			}

			/* TODO: escape ":" */

			result = realloc(result, result_len + strlen(child_node->string) + 1);
			if (!result)
				return NULL;

			strcpy(result + result_len, child_node->string);
			if (result_len > 0)
				result[result_len - 1] = ':';
			result_len += strlen(child_node->string) + 1;
		}

		return result;
	}

	for (int i = 0; i < this_node->nr_child; ++i) {
		string_tree_node_t *child_node;

		child_node = string_tree_get_child_node(this_node, i);
		*associated = 0;
		char *result = query_string_tree(child_node, path, associated);
		if (result)
			return result;
		else if (*associated)
			break;
	}

	/* Not found */
	return NULL;
}

char *
string_tree_query(const char *path)
{
	string_tree_node_t *this_node = &string_tree_root;

	if (!this_node->nr_child)
		return NULL;

	int len = strlen(string_tree_root.string);
	if (strncmp(this_node->string, path, len))
		return NULL;

	path += len;

	for (unsigned int i = 0; i < this_node->nr_child; ++i) {
		string_tree_node_t *child_node;
		int associated = 0;

		child_node = __string_tree_get_child_node(this_node, i);
		char *result = query_string_tree(child_node, path, &associated);
		if (result)
			return result;
		else if (associated)
			break;
	}

	return NULL;
}
