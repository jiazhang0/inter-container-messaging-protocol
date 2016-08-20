/*
 * Inter-Container Messaging Protocol Daemon (ICMPD)
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

static int opt_quite;
static char *opt_log_file, *opt_conf_file;

static void
show_banner(void)
{
	info_cont("\nInter-Container Messaging Protocol v%d Daemon\n",
		  ICMP_VERSION);
	info_cont("(C)Copyright 2016, Lans Zhang\n");
	info_cont("Version: %s+git%s\n", VERSION, ic_git_commit);
	info_cont("Build Machine: %s\n", ic_build_machine);
	info_cont("Build Time: " __DATE__ " " __TIME__ "\n\n");
}

static void
show_version(void)
{
	info_cont("%s\n", VERSION);
}

static void
show_usage(const char *prog)
{
	info_cont("Usage: %s <options> <subcommand> [<args>]\n",
		  prog);
	info_cont("\noptions:\n");
	info_cont("  --help, -h: Print this help information\n");
	info_cont("  --version, -V: Show version number\n");
	info_cont("  --verbose, -v: Show verbose messages\n");
	info_cont("  --quite, -q: Don't show banner information\n");
	info_cont("\nsubcommand:\n");
	info_cont("  help: Display the help information for the "
		  "specified command\n");
	info_cont("  start: Launch the deamon\n");
	info_cont("\nargs:\n");
	info_cont("  Run `%s help <subcommand>` for the details\n", prog);
}

static int
parse_options(int argc, char *argv[])
{
	char opts[] = "-hVvqlc";
	struct option long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "quite", no_argument, NULL, 'q' },
		{ "log-file", required_argument, NULL, 'l' },
		{ "config-file", required_argument, NULL, 'c' },
		{ 0 },	/* NULL terminated */
	};

	while (1) {
		int opt, index;

		opt = getopt_long(argc, argv, opts, long_opts, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case '?':
			err("Unrecognized option\n");
			return -1;
		case 'h':
			show_usage(argv[0]);
			exit(EXIT_SUCCESS);
		case 'V':
			show_version();
			exit(EXIT_SUCCESS);
		case 'v':
			ic_util_set_verbosity(1);
			break;
		case 'q':
			opt_quite = 1;
			break;
		case 'l':
			opt_log_file = optarg;
			break;
		case 'c':
			opt_conf_file = optarg;
			break;
		case 1:
			index = optind;
			optind = 1;
			if (subcommand_parse(argv[0], optarg, argc - index + 1,
					  argv + index - 1)) 
				exit(EXIT_FAILURE);
			return 0;
		default:
			return -1;
		}
	}

	return 0;
}

extern subcommand_t subcommand_help;
extern subcommand_t subcommand_start;

static void
exit_notify(void)
{
	info("icmpd (%ld) exiting with %d (%s)\n", gettid(), errno,
	     strerror(errno));
}

int
main(int argc, char *argv[], char *envp[])
{
	atexit(exit_notify);

	dbg("Dump icmpd cmdline:\n");
	for (int i = 0; i < argc; ++i)
		dbg_cont("  [%d]%s\n", i, argv[i]);

	/*
	 * For static link which doesn't link constructor/destructor if
	 * without explicitly call them.
	 */
	//libsel_init();

	subcommand_add(&subcommand_help);
	subcommand_add(&subcommand_start);

	int rc = parse_options(argc, argv);
	if (rc)
		return rc;

	if (!opt_quite)
		show_banner();

	return subcommand_run_current();
}
