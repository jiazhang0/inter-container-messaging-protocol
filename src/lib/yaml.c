/*
 * Yaml parser
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#include <yaml.h>
#include <eee.h>
#include "conf_file.h"
#include "string_tree.h"

#ifndef YAML_ERRNO_BASE
  #error "YAML_ERRNO_BASE isn't defined"
#endif

static int
walk_through_yaml(yaml_document_t *document, yaml_node_t *node,
		  string_tree_node_t *str_node)
{
	switch (node->type) {
	case YAML_SCALAR_NODE: {
		const char *str = (const char *)node->data.scalar.value;
		int str_len = node->data.scalar.length;

		/* e.g, '*' */
		if (node->data.scalar.style != YAML_PLAIN_SCALAR_STYLE)
			dbg("not YAML_PLAIN_SCALAR_STYLE\n");

		if (str_node->string) {
			string_tree_create_child_node(str_node, 1);
			str_node = __string_tree_get_child_node(str_node, 0);
		}

		string_tree_set_node_string(str_node, str, str_len);

		dbg("Found scalar node: %s\n", string_tree_get_node_string(str_node));

		break;
	}
	case YAML_MAPPING_NODE: {
		yaml_node_pair_t *start = node->data.mapping.pairs.start;
		yaml_node_pair_t *end = node->data.mapping.pairs.top;
		unsigned int len = end - start;

		dbg("Found %d mapping nodes\n", len);

		string_tree_create_child_node(str_node, len);

		int i = 0;
		for (yaml_node_pair_t *pair = start; pair < end; ++pair, ++i) {
			yaml_node_t *key = yaml_document_get_node(document, pair->key);
			yaml_node_t *val = yaml_document_get_node(document, pair->value);

			dbg("Parsing key ...\n");
			int rc = walk_through_yaml(document, key,
						   __string_tree_get_child_node(str_node, i));
			if (rc)
				return -1;

			dbg("Parsing val ...\n");
			rc = walk_through_yaml(document, val,
					       __string_tree_get_child_node(str_node, i));
			if (rc)
				return -1;
		}
		break;
	}
	case YAML_SEQUENCE_NODE: {
		yaml_node_item_t *start = node->data.sequence.items.start;
		yaml_node_item_t *end = node->data.sequence.items.top;
		unsigned int len = end - start;

		dbg("Found %d sequence nodes\n", len);

		if (!len)
			break;

		string_tree_create_child_node(str_node, len);

		int i = 0;
		for (yaml_node_item_t *item = start; item < end; ++item, ++i) {
			yaml_node_t *elm = yaml_document_get_node(document, *item);
			int rc = walk_through_yaml(document, elm,
						   __string_tree_get_child_node(str_node, i));
			if (rc)
				return -1;
		}

		break;
	}
	case YAML_NO_NODE:
		dbg("Empty node\n");
		break;
	default:
		dbg("Unknown node\n");
	}

	return 0;
}

int
ic_yaml_conf_parse(string_tree_node_t *root_node,
		   char *conf_file)
{
	/*¡¡Note 1 signifies success, 0 failure returned by libyaml */
	int rc;

	yaml_parser_t parser;
	rc = yaml_parser_initialize(&parser);
	if (!rc) {
		err("Unable to parse %s\n", conf_file);
		return -1;
	}

	FILE *fp = fopen(conf_file, "r");
	if (!fp) {
		err("Unable to open %s\n", conf_file);
		return -1;
	}

	yaml_parser_set_input_file(&parser, fp);

	yaml_document_t document;
	if (!yaml_parser_load(&parser, &document))
		die("YAML parser error %d\n", parser.error);

	yaml_node_t *root = yaml_document_get_root_node(&document);
	rc = walk_through_yaml(&document, root, root_node);
	if (rc)
		die("YAML walk error\n");

	yaml_document_delete(&document);
	yaml_parser_delete(&parser);
	fclose(fp);

	return 0;
}
