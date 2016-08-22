/*
 * ICMPD start sub-command
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

typedef struct {
	char *container_name;
	int socket;
	int nr_monitored_container;
	char **monitored_container;
	ic_transport_t self_transport;
	vector_t *monitored_worker;
} icmpd_context_t;

#define ICMPD_DEFAULT_CONF_FILE		"/etc/icmpd.conf"
#define ICMPD_DEFAULT_LOG_FILE		"/var/log/icmpd.log"

static char *opt_conf_file = ICMPD_DEFAULT_CONF_FILE;
static char *opt_log_file;
static int opt_daemon;

static int
init_context(icmpd_context_t *ctx)
{
	ctx->container_name = ic_container_name();
	if (!ctx->container_name)
		return -1;

	char *monitored_container = ic_conf_file_query(".%s.monitor",
						  ctx->container_name);
	if (!monitored_container) {
		if (strcmp(ctx->container_name, "container-essential")) {
			err(".%s.monitor is not configured\n", ctx->container_name);
			goto err_monitored_container;
		}

		/* The default configuration file always uses host to
		 * identify essential system.
		 */
		monitored_container = ic_conf_file_query(".host.monitor");
		if (!monitored_container) {
			err(".host.monitor is not configured\n");
			goto err_monitored_container;
		}
	}

	if (strcmp(monitored_container, "*")) {
		ctx->monitored_container = ic_util_split_string(monitored_container, ":",
							   (unsigned int *)&ctx->nr_monitored_container);
		if (!ctx->monitored_container) {
			err("Failed to retrieve the setting of .%s.monitor",
			    ctx->container_name);
			goto err_monitored_container_list;
		}
	} else {
		/* FIXME: load the global configuration file to get the lxcpath */
		const char *lxcpath = IC_DEFAULT_LXC_PATH;

		vector_t *vec = ic_lxc_list_defined_containers(lxcpath);
		if (!vec) {
			err("Failed to list the defined lxc containers "
			    "in %s\n", lxcpath);
			goto err_monitored_container_list;
		}

		ctx->monitored_container = NULL;
		ctx->nr_monitored_container = vector_to_string_array(vec, &ctx->monitored_container);
		vector_destroy(vec);
		if (ctx->nr_monitored_container < 0)
			goto err_monitored_container_list;
	}

	eee_mfree(monitored_container);

	if (!ctx->nr_monitored_container)
		warn("No monitored containers\n");
	else if (ic_util_verbose()) {
		info("Monitored containers (%d):\n", ctx->nr_monitored_container);
		for (int i = 0; i < ctx->nr_monitored_container; ++i)
			info_cont(" %s", ctx->monitored_container[i]);
		info_cont("\n");
	}

	return 0;

err_monitored_container_list:
	eee_mfree(monitored_container);

err_monitored_container:
	eee_mfree(ctx->container_name);

	return -1;
}

/* Construct argv[] for execvp() */
static int
build_argv(const char *argument, char ***ret_argv, char **ret_args)
{
	char *args = malloc(eee_strlen(argument) + 1);
	ic_assert(args, "Unable to allocate argument");

	eee_memcpy(args, argument, eee_strlen(argument) + 1);

	char **argv = (char **)malloc(sizeof(char *));
	ic_assert(argv, "Unable to allocate argv");

	int argc = 0;
	argv[0] = NULL;
	char *curr_arg = args;
	while (*curr_arg) {
		char *prev_arg;

		/* Skip preposed spaces */
		while (*curr_arg && isspace(*curr_arg))
			++curr_arg;

		if (*curr_arg)
			prev_arg = curr_arg++;
		else
			break;

		/* Skip characters */
		while (*curr_arg && !isspace(*curr_arg))
			++curr_arg;

		argv = eee_mrealloc(argv, 0, sizeof(char *) * (++argc + 1));
		ic_assert(argv, "Failed to re-allocate argv");

		argv[argc - 1] = prev_arg;
		argv[argc] = NULL;

		if (*curr_arg)
			*(curr_arg++) = 0;
	}

	*ret_argv = argv;
	*ret_args = args;

	return 0;
}

static int
echo_data(void *ctx, const char *echo_data, unsigned long echo_data_len)
{
	ic_transport_t tr = (ic_transport_t)ctx;
#ifdef DEBUG
	const char *name = ic_transport_name(tr);
#endif

	dbg("Execute echo: %s (%ld-byte)\n", (char *)echo_data,
	    echo_data_len);

	dbg("Preparing to send ICMP response message to %s ...\n", name);

	void *msg;
	unsigned long msg_len;
	int rc = icmp_marshal((void *)echo_data, echo_data_len, ICMP_CC_ECHO,
			      &msg, &msg_len);
	ic_assert(!rc, "Unable to marshal ICMP message");

	rc = ic_transport_send_data(tr, msg, msg_len);
	eee_mfree(msg);
	if (rc) {
		err("Failed to send ICMP request message");
		return rc;
	}

	dbg("%ld-byte ICMP response message sent to %s\n", msg_len, name);

	return rc;
}

static int
execute_cmd(void *ctx, const char *cmdline, unsigned long cmdline_len)
{
	ic_transport_t tr = (ic_transport_t)ctx;
#ifdef DEBUG
	const char *name = ic_transport_name(tr);
#endif

	dbg("Execute commandline: %s (%ld-byte)\n", (char *)cmdline,
	    cmdline_len);

	/* Create two pipelines for reading and writing */

	int input_fds[2];
	int rc = pipe(input_fds);
	ic_assert(rc >= 0, "Error creating the pipe for input");

	int output_fds[2];
	rc = pipe(output_fds);
	ic_assert(rc >= 0, "Error creating the pipe for output");

	pid_t child = fork();
	ic_assert((int)child >= 0, "Error forking subprocess");

	if (!child) {
		char **argv;
		char *args;
		rc = build_argv(cmdline, &argv, &args);
		ic_assert(!rc, "Unable to build argv");

		close(input_fds[1]);
		close(output_fds[0]);

		/* Bind the standard input to the input endpoint of
		 * input pipe.
		 */
		ic_assert(dup2(input_fds[0], STDIN_FILENO) == STDIN_FILENO,
			  "Unable to dup to stdin");
		/* Bind the standard output and error to the output
		 * endpoint of output pipe.
		 */
		ic_assert(dup2(output_fds[1], STDOUT_FILENO) == STDOUT_FILENO,
			  "Unable to dup to stdout");
		ic_assert(dup2(output_fds[1], STDERR_FILENO) == STDERR_FILENO,
			  "Unable to dup to stderr");

		close(input_fds[0]);
		close(output_fds[1]);

		execvp(argv[0], argv);

		/* Should not return */
		ic_assert(0, "Error executing subprocess %s", argv[0]);
	}

	close(input_fds[0]);
	close(output_fds[1]);

	/* TODO: Rediretct the transport to the stdin */
	if (0) {
		while (cmdline_len) {
			ssize_t sz = write(input_fds[1], cmdline, cmdline_len);
			if (sz < 0 && errno == EPIPE)
				break;

			ic_assert(sz >= 0, "Unable to write to stdin");

			if (sz == cmdline_len)
				break;

			cmdline += sz;
			cmdline_len -= sz;
		}
	}

	close(input_fds[1]);

	buffer_stream_t bs;
	bs_init(&bs, NULL, 0);

	/* Rediretct stdout and stderr to the transport */
	rc = 0;
	while (1) {
		/* TODO: get the remaining size in the pipe */
        	char data[PIPE_BUF];

		ssize_t sz = read(output_fds[0], data, sizeof(data));
		ic_assert(sz >= 0, "Unable to read from stdout/stderr");

		if (!sz)
			break;

		bs_reserve(&bs, sz);
		bs_post_put(&bs, data, sz);
	}

	/* Add a NULL charactor in order to make the result printable
	 * directly for icmpc.
	 */
	bs_reserve(&bs, 1);
	char nul = 0;
	bs_post_put(&bs, &nul, 1);

	dbg("Preparing to send ICMP response message to %s ...\n", name);

	void *msg;
	unsigned long msg_len;
	rc = icmp_marshal(bs_head(&bs), bs_size(&bs), ICMP_CC_COMMMANDLINE,
			  &msg, &msg_len);
	bs_destroy(&bs);
	ic_assert(!rc, "Unable to marshal ICMP message");

	rc = ic_transport_send_data(tr, msg, msg_len);
	eee_mfree(msg);
	if (rc) {
		err("Failed to send ICMP request message");
		return rc;
	}

	dbg("%ld-byte ICMP response message sent to %s\n", msg_len, name);

	close(output_fds[0]);
	waitpid(child, NULL, 0);

	return rc;
}

static int
check_limited_commands(const char *cmd)
{
	char *local_container_name = ic_container_name();
	int rc = -1;

	if (!local_container_name)
		return -1;

	char *limited_commands = ic_conf_file_query(".%s.commands",
						    local_container_name);
	if (!limited_commands) {
		if (strcmp(local_container_name, "container-essential")) {
			info("%s.commands is not configured\n",
			    local_container_name);
			eee_mfree(local_container_name);
			return 0;
		}

		eee_mfree(local_container_name);

		/* In case of essential, check the setting for host */
		limited_commands = ic_conf_file_query(".host.commands");
		if (!limited_commands) {
			info("host.commands is not configured\n");
			return 0;
		}

		local_container_name = strdup("host");
		if (!local_container_name) {
			eee_mfree(limited_commands);
			return -1;
		}
	}

	if (!strcmp(limited_commands, "*")) {
		info("%s is not limited by %s.commands\n", cmd,
		    local_container_name);
		eee_mfree(limited_commands);
		eee_mfree(local_container_name);

		return 0;
	}

	unsigned int nr_cmd = 0;
	char **splitted_cmds = ic_util_split_string(limited_commands, ":",
						    (unsigned int *)&nr_cmd);
	eee_mfree(limited_commands);
	if (!splitted_cmds) {
		err("Failed to retrieve the setting of %s.commands\n",
		    local_container_name);
		goto err_splitted_cmds;
	}

	unsigned int i;
	for (i = 0; i < nr_cmd; ++i) {
		if (!strncmp(splitted_cmds[i], cmd, strlen(splitted_cmds[i]))) {
			info("%s is limited by %s.commands\n", cmd,
			    local_container_name);
			rc = 0;
			break;
		}
	}

	eee_mfree(splitted_cmds);

	if (i == nr_cmd) {
		warn("%s is prohibited by %s.commands\n", cmd,
		     local_container_name);
		ic_set_errno(IC_ERRNO_COMMAND_DENIED);
	}

err_splitted_cmds:
	eee_mfree(local_container_name);

	return rc;
}

static int
check_command_acl(const char *cmd, const char *target_container_name)
{
	char *acl_list;

	acl_list = ic_conf_file_query(".commands.%s.acl", cmd);
	if (!acl_list) {
		info("commands.%s.acl is not configured\n", cmd);
		return 0;
	}

	if (!strcmp(acl_list, "*")) {
		info("%s is globally granted\n", cmd);
		eee_mfree(acl_list);
		return 0;
	}

	unsigned int nr_acl_list = 0;
	char **splitted_acl_list = ic_util_split_string(acl_list, ":",
							(unsigned int *)&nr_acl_list);
	eee_mfree(acl_list);
	if (!splitted_acl_list) {
		err("Failed to retrieve the setting of commands.%s.acl\n",
		    cmd);
		return -1;
	}

	unsigned int i;
	for (i = 0; i < nr_acl_list; ++i) {
		if (!strcmp(splitted_acl_list[i], target_container_name)) {
			info("%s is granted by commands.%s.acl for %s\n", cmd,
			     cmd, target_container_name);
			break;
		}
	}

	eee_mfree(splitted_acl_list);

	if (i == nr_acl_list) {
		warn("%s is not granted by commands.%s.acl for %s\n", cmd,
		    cmd, target_container_name);
		ic_set_errno(IC_ERRNO_COMMAND_DENIED);
		return -1;
	}

	return 0;
}

static int
check_command(void *ctx, const char *cmdline, unsigned long cmdline_len)
{
	/* space, form-feed ('\f'), newline ('\n'), carriage return ('\r'),
	 * horizontal tab ('\t'), and vertical tab ('\v').
	 */
	const char spaces[] = " \f\n\r\t\v";

	/* Extract the program name */
	cmdline_len = strcspn(cmdline, spaces) + 1;
	char *cmd = strndup(cmdline, cmdline_len);
	if (!cmd)
		return -1;

	cmd[cmdline_len - 1] = 0;

	int rc = check_limited_commands(cmd);
	if (rc) {
		eee_mfree(cmd);
		return rc;
	}

	ic_transport_t tr = (ic_transport_t)ctx;
	rc = check_command_acl(cmd, ic_transport_name(tr));
	eee_mfree(cmd);

	return rc;
}

static int
handle_payload(void *ctx, uint16_t cc, const void *payload,
	       unsigned long payload_len)
{
	int rc = -1;

	switch (cc) {
	case ICMP_CC_ECHO:
		rc = echo_data(ctx, payload, payload_len);
		break;
	case ICMP_CC_COMMMANDLINE:
		rc = check_command(ctx, (const char *)payload, payload_len);
		if (!rc)
			rc = execute_cmd(ctx, (const char *)payload,
					 payload_len);
		else if (ic_get_errno() == IC_ERRNO_COMMAND_DENIED)
			rc = 0;
		break;
	default:
		err("Unknown command code: 0x%x\n", cc);
	}

	return rc;
}

static int
handle_protocol(ic_transport_t tr)
{
#ifdef DEBUG
	const char *name = ic_transport_name(tr);
#endif

	while (1) {
		dbg("Preparing to receive ICMP request message from %s ...\n",
		    name);

		void *msg = NULL;
		unsigned long msg_len = 0;

		int rc = ic_transport_receive_data(tr, &msg, &msg_len);
		if (rc) {
			err("Failed to receive ICMP request message from "
			    "self transport\n");
			break;
		}

		dbg("%ld-byte ICMP request message from %s received\n",
		    msg_len, name);

		rc = icmp_unmarshal(msg, msg_len, ICMP_CC_NOT_SPECIFIED,
				    (int (*)(void *, uint16_t, const void *, unsigned long))handle_payload,
				    (void *)tr);
		ic_transport_free_data(tr, msg);
		if (rc) {
			err("Failed to unmarshal ICMP response message\n");
			return rc;
		}
	}

	return 0;
}

static int __attribute__((unused))
create_thread_worker(ic_transport_t tr)
{
	pthread_t thread;
	pthread_attr_t attr;
	int rc;

	pthread_attr_init(&attr);
	rc = pthread_create(&thread, &attr, (void *(*)(void *))handle_protocol,
			    (void *)tr);
	if (rc) {
		err("Failed to create thread for %s\n", ic_transport_name(tr));
		return rc;
	}

	return 0;
}

static void
worker_exit(int sig)
{
	info("Worker (%ld) passively exiting ...\n", gettid());
	exit(EXIT_SUCCESS);
}

static pid_t
create_worker(const char *name)
{
	dbg("Preparing to create worker for %s\n", name);

	/* Prevent from writing the closed pipe */
	ic_assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR,
		  "Unable to register SIGPIPE");

	ic_assert(signal(SIGTERM, worker_exit) != SIG_ERR,
		  "Unable to set up SIGTERM");

	ic_assert(signal(SIGINT, worker_exit) != SIG_ERR,
		  "Unable to set up SIGINT");

	pid_t child = fork();
	ic_assert((int)child >= 0, "Error forking worker for %s", name);

	if (!child) {
		ic_transport_t tr;
		tr = ic_transport_create_master(name);
		if (!tr)
			exit(EXIT_FAILURE);

		ic_garbage_register((void *)tr, ic_transport_destroy);

		info("icmpd worker (%ld) for %s created\n", gettid(), name);

		while (1)
			handle_protocol(tr);

		ic_transport_destroy(tr);
		exit(EXIT_SUCCESS);
	}

	return child;
}

static void
stop_worker(vector_t *vec)
{
	unsigned int nr_vec = vector_get_nr_vector(vec);

	for (unsigned int i = 0; i < nr_vec; ++i) {
		pid_t *child;

		child = vector_get_obj(vec, i);
		/* If the monitored container contains local container, then the pid of
		 * the local container is not recorded.
		 */
		if (!*child)
			continue;

		kill(*child, SIGUSR1);
		waitpid(*child, NULL, 0);
	}
}

static int
create_transport(icmpd_context_t *ctx)
{
	if (ctx->nr_monitored_container > 0) {
		ctx->monitored_worker = vector_create(ctx->nr_monitored_container,
						      sizeof(pid_t),
						      NULL, NULL, NULL,
						      VECTOR_FLAGS_DEFAULT);
		if (!ctx->monitored_worker) {
			err("Failed to create vector for monitored workers\n");
			goto err_monitored_worker;
		}
	}

	int i = 0;
	for (; i < ctx->nr_monitored_container; ++i) {
		pid_t child;

		if (!strcmp(ctx->monitored_container[i], ctx->container_name)) {
			child = (pid_t)0;
			vector_set_obj(ctx->monitored_worker, i, &child,
				       sizeof(child));
			continue;
		}

		child = create_worker(ctx->monitored_container[i]);
		if ((int)child < 0)
			goto err_create_worker;

		vector_set_obj(ctx->monitored_worker, i, &child, sizeof(child));
	}

#if 0
	ic_transport_t self_tr;

	self_tr = ic_transport_create_master(ctx->container_name);
	if (!self_tr)
		goto err_create_worker;

	ic_garbage_register((void *)self_tr, ic_transport_destroy);
	ctx->self_transport = self_tr;
#endif

	return 0;

err_create_worker:
	while (i)
		stop_worker(ctx->monitored_worker);

	if (ctx->nr_monitored_container > 0)
		vector_destroy(ctx->monitored_worker);

err_monitored_worker:
//	ic_transport_destroy(self_tr);

	return -1;
}

/*
 * Daemonlize icmpd.
 * - Change CWD to /.
 * - Redirect stdin to /dev/null.
 * - Redirect stdout and stderr to log file.
 */
static int
daemonlize(void)
{
	if (opt_daemon)
		ic_assert(!daemon(0, 1), "Failed to daemonlize");

	if (!opt_log_file)
		return 0;

	int log_fd = open(opt_log_file,
			  O_WRONLY | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR | S_IRGRP);
	ic_assert(log_fd > 0, "Failed to open log file %s", opt_log_file);

	int null_fd = open("/dev/null", O_RDWR);
	ic_assert(null_fd > 0,
		  "Unable to open /dev/null");

	ic_assert(dup2(null_fd, STDIN_FILENO) == STDIN_FILENO,
		  "Unable to redirect stdin to /dev/null");
	close(null_fd);

	ic_assert(dup2(log_fd, STDOUT_FILENO) == STDOUT_FILENO,
		  "Unable to redirect stdout to log file");
	ic_assert(dup2(log_fd, STDERR_FILENO) == STDERR_FILENO,
		  "Unable to redirect stderr to log file");
	close(log_fd);

	return 0;
}

static void
show_usage(char *prog)
{
	info_cont("\nUsage: %s start <args>\n", prog);
	info_cont("Start the container protocol daemon.\n");
	info_cont("\nargs:\n");
	info_cont("  --log-file, -l: (optional) Log file, e.g, "
		  ICMPD_DEFAULT_LOG_FILE ".\n");
	info_cont("  --config-file, -c: Configuration file. The default is "
		  ICMPD_DEFAULT_CONF_FILE ".\n");
	info_cont("  --fg, -f: (optional) Run in foreground.\n");
}

static int
parse_arg(int opt, char *optarg)
{
	switch (opt) {
	case 'l':
		opt_log_file = optarg;
		break;
	case 'c':
		opt_conf_file = optarg;
		break;
	case 'd':
		opt_daemon = 1;
		break;
	case 1:
	default:
		return -1;
	}

	return 0;
}

static int
run_start(char *prog)
{
	int rc;

	rc = daemonlize();
	if (rc < 0)
		return rc;

	rc = ic_conf_file_parse(opt_conf_file);
	if (rc)
		goto err_conf_file_parse;

	icmpd_context_t ctx;
	rc = init_context(&ctx);
	if (rc)
		goto err_init_context;

	rc = create_transport(&ctx);
	if (rc)
		goto err_create_transport;

	//return handle_protocol(ctx.self_transport);
	while (1) pause();

err_create_transport:
err_init_context:
err_conf_file_parse:

	return rc;
}

static struct option long_opts[] = {
	{ "config-file", required_argument, NULL, 'c' },
	{ "log-file", required_argument, NULL, 'l' },
	{ "opt_daemon", no_argument, NULL, 'd' },
	{ 0 },	/* NULL terminated */
};

subcommand_t subcommand_start = {
	.name = "start",
	.optstring = "-l:c:d",
	.long_opts = long_opts,
	.parse_arg = parse_arg,
	.show_usage = show_usage,
	.run = run_start,
};
