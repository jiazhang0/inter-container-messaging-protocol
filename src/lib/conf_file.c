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

#include <ic.h>
#include "conf_file.h"
#include "string_tree.h"

int
ic_conf_file_parse(char *conf_file)
{
	assert(!access(conf_file, R_OK));

	int rc = ic_yaml_conf_parse(&string_tree_root, conf_file);
	if (rc)
		return rc;

	if (ic_util_verbose())
		string_tree_dump_tree(&string_tree_root);

	return 0;
}

char *
ic_conf_file_query(const char *fmt, ...)
{
	va_list ap;
	char query[PATH_MAX];

	va_start(ap, fmt);
	vsnprintf(query, sizeof(query) - 1, fmt, ap);
	va_end(ap);

	char *result = string_tree_query(query);
	if (!result)
		dbg("Unable to retrieve the result for the query %s\n", query);
	else
		dbg("The query result: %s\n", result);

	return result;
}
