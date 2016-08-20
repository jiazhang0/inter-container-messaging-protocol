/*
 * Configuration file abstraction
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef __CONF_FILE_H__
#define __CONF_FILE_H__

#include <ic.h>
#include "string_tree.h"

#ifndef IC_CONF_ERRNO_BASE
  #error "IC_CONF_ERRNO_BASE isn't defined"
#endif

typedef enum {
	IC_CONF_FILE_PATH_TYPE_MAPPING,
	IC_CONF_FILE_PATH_TYPE_SEQUENCE,
	IC_CONF_FILE_PATH_TYPE_STRING,
} ic_conf_file_path_type_t;

struct ic_conf_file_path {
	ic_conf_file_path_type_t type;
	char *key;
	void *val;
	unsigned int nr_sub_path;
	struct ic_conf_file_path *parent;
};

typedef struct ic_conf_file_path ic_conf_file_path_t;

typedef struct {
	int (*create_sub_path)(ic_conf_file_path_t *parent,
			       unsigned int nr_sub_path,
			       ic_conf_file_path_type_t type);
	int (*set_path_key)(ic_conf_file_path_t *path, const char *str,
			    unsigned int str_len);
	int (*set_path_value)(ic_conf_file_path_t *path, const char *str,
			      unsigned int str_len);
} ic_conf_file_path_parser_t;

extern int
ic_yaml_conf_parse(string_tree_node_t *root_node,
		   char *conf_file);

#endif	/* __CONF_FILE_H__ */
