/*
 * Error status definitions
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef SUBCOMMAND_H
#define SUBCOMMAND_H

#include <eee.h>

typedef struct {
	const char *name;
	const char *optstring;
	const struct option *long_opts;
	int (*parse_arg)(int opt, char *optarg);
	void (*show_usage)(char *prog);
	int (*run)(char *prog);
} subcommand_t;

extern int
subcommand_add(subcommand_t *subcmd);

extern subcommand_t *
subcommand_find(char *subcmd);

extern int
subcommand_parse(char *prog, char *subcmd, int argc, char *argv[]);

extern int
subcommand_run_current(void);

#endif	/* SUBCOMMAND_H */
