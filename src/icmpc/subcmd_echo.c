/*
 * ICMPC echo sub-command
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
} icmpc_context_t;

#define ICMPC_DEFAULT_CONF_FILE		"/etc/icmpc.conf"

static char *opt_conf_file;
static char *opt_echo_data;
static char *opt_requestor;

static int
init_context(icmpc_context_t *ctx)
{
	if (!opt_requestor)
		ctx->container_name = strdup("local");
	else
		ctx->container_name = strdup(opt_requestor);

	if (!ctx->container_name)
		return -1;

	return 0;
}

static void
destroy_context(icmpc_context_t *ctx)
{
	eee_mfree(ctx->container_name);
}

static int
print_result(void *context, uint16_t cc, const void *data,
	     unsigned long data_len)
{
	fprintf(stdout, "%s\n", (char *)data);
	fflush(stdout);

	return 0;
}

static int
handle_protocol(icmpc_context_t *ctx, char *echo_data, char *requestor)
{
	void *msg;
	unsigned long msg_len;
	unsigned long echo_data_len = strlen(echo_data) + 1;

	int rc = icmp_marshal(echo_data, echo_data_len, ICMP_CC_ECHO,
			      &msg, &msg_len);
	if (rc) {
		err("Failed to marshal ICMP request message\n");
		return rc;
	}

	ic_transport_t tr = ic_transport_create_slave(requestor);
	if (!tr) {
		err("Unable to create the slave transport for %s\n",
		    requestor);
		return -1;
	}

	dbg("Preparing to send ICMP request message ...\n");

	rc = ic_transport_send_data(tr, msg, msg_len);
	eee_mfree(msg);
	if (rc) {
		err("Failed to send ICMP request message\n");
		goto err_send_data;
	}

	dbg("%ld-byte ICMP request message with %ld-byte payload sent\n",
	    msg_len, echo_data_len);

	dbg("Preparing to receive ICMP response message ...\n");

	msg = NULL;
	msg_len = 0;
	rc = ic_transport_receive_data(tr, &msg, &msg_len);
	if (rc) {
		err("Failed to receive ICMP response message\n");
		goto err_send_data;
	}

	dbg("%ld-byte ICMP response message received\n", msg_len);

	rc = icmp_unmarshal(msg, msg_len, ICMP_CC_ECHO,
			    print_result, ctx);
	ic_transport_free_data(tr, msg);
	ic_transport_destroy(tr);
	if (rc)
		err("Failed to unmarshal ICMP response message\n");

	return rc;

err_send_data:
	ic_transport_destroy(tr);

	return rc;
}

static void
show_usage(char *prog)
{
	info_cont("\nUsage: %s run_container <command> <args>\n", prog);
	info_cont("Container management. Currently only LXC is supported.\n");
	info_cont("\nargs:\n");
	info_cont("  --config-file, -c: (optional) Configuration file. "
		  "The default is " ICMPC_DEFAULT_CONF_FILE ".\n");
	info_cont("  --requestor, -r: (optional) Set the command "
		  "requestor. The default is local.\n");
}

static int
parse_arg(int opt, char *optarg)
{
	switch (opt) {
	case 'c':
		opt_conf_file = optarg;
		break;
	case 'r':
		opt_requestor = optarg;
		break;
	case 1:
		opt_echo_data = optarg;
		break;
	default:
		return -1;
	}

	return 0;
}

static int
run_echo(char *prog)
{
	int rc;

	if (!opt_echo_data)
		return -1;

	if (opt_conf_file) {
		rc = ic_conf_file_parse(opt_conf_file);
		if (rc < 0)
			return rc;
	}

	icmpc_context_t ctx;
	init_context(&ctx);

	rc = handle_protocol(&ctx, opt_echo_data, ctx.container_name);
	destroy_context(&ctx);

	return rc;
}

static struct option long_opts[] = {
	{ "config-file", required_argument, NULL, 'c' },
	{ "requestor", required_argument, NULL, 'r' },
	{ 0 },	/* NULL terminated */
};

subcommand_t subcommand_echo = {
	.name = "echo",
	.optstring = "-c:r:",
	.long_opts = long_opts,
	.parse_arg = parse_arg,
	.show_usage = show_usage,
	.run = run_echo,
};
