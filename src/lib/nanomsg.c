/*
 * Nanomsg infrastructure
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#include "nanomsg.h"

#define nn_print_error(msg)	\
	do {	\
		int __errno__ = nn_errno();	\
		err(msg ": -%d (%s)\n", __errno__, \
		    nn_strerror(__errno__)); \
	} while (0)

/* For generic socket-level options */
static void
show_generic_options(int s)
{
	dbg("[Generic socket-level options]\n");

	int val_int;
	size_t sz = sizeof(val_int);
	int rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_DOMAIN, &val_int, &sz);
	if (!rc)
		dbg("NN_DOMAIN: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_DOMAIN");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_PROTOCOL, &val_int, &sz);
	if (!rc)
		dbg("NN_PROTOCOL: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_PROTOCOL");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_LINGER, &val_int, &sz);
	if (!rc)
		dbg("NN_LINGER: %d milliseconds\n", val_int);
	else
		nn_print_error("Unable to access NN_LINGER");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_SNDBUF, &val_int, &sz);
	if (!rc)
		dbg("NN_SNDBUF: %d bytes\n", val_int);
	else
		nn_print_error("Unable to access NN_SNDBUF");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RCVBUF, &val_int, &sz);
	if (!rc)
		dbg("NN_RCVBUF: %d bytes\n", val_int);
	else
		nn_print_error("Unable to access NN_RCVBUF");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RCVMAXSIZE, &val_int, &sz);
	if (!rc) {
		if (val_int >= 0)
			dbg("NN_RCVMAXSIZE: %d bytes\n", val_int);
		else
			dbg("NN_RCVMAXSIZE: unlimited\n");
	} else
		nn_print_error("Unable to access NN_RCVMAXSIZE");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_SNDTIMEO, &val_int, &sz);
	if (!rc) {
		if (val_int >= 0)
			dbg("NN_SNDTIMEO: %d milliseconds\n", val_int);
		else
			dbg("NN_SNDTIMEO: infinite\n");
	} else
		nn_print_error("Unable to access NN_SNDTIMEO");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RCVTIMEO, &val_int, &sz);
	if (!rc) {
		if (val_int >= 0)
			dbg("NN_RCVTIMEO: %d milliseconds\n", val_int);
		else
			dbg("NN_RCVTIMEO: infinite\n");
	} else
		nn_print_error("Unable to access NN_RCVTIMEO");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RECONNECT_IVL, &val_int, &sz);
	if (!rc)
		dbg("NN_RECONNECT_IVL: %d milliseconds\n", val_int);
	else
		nn_print_error("Unable to access NN_RECONNECT_IVL");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RECONNECT_IVL_MAX, &val_int, &sz);
	if (!rc)
		dbg("NN_RECONNECT_IVL_MAX: %d milliseconds\n", val_int);
	else
		nn_print_error("Unable to access NN_RECONNECT_IVL_MAX");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_SNDPRIO, &val_int, &sz);
	if (!rc)
		dbg("NN_SNDPRIO: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_SNDPRIO");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RCVPRIO, &val_int, &sz);
	if (!rc)
		dbg("NN_RCVPRIO: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_RCVPRIO");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_IPV4ONLY, &val_int, &sz);
	if (!rc)
		dbg("NN_IPV4ONLY: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_IPV4ONLY");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_SNDFD, &val_int, &sz);
	if (!rc)
		dbg("NN_SNDFD: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_SNDFD");

	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_RCVFD, &val_int, &sz);
	if (!rc)
		dbg("NN_RCVFD: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_RCVFD");

	char name[64];
	sz = sizeof(name);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_SOCKET_NAME, name, &sz);
	if (!rc)
		dbg("NN_SOCKET_NAME: %s\n", name);
	else
		nn_print_error("Unable to access NN_SOCKET_NAME");

#if 0
	sz = sizeof(val_int);
	rc = nn_getsockopt(s, NN_SOL_SOCKET, NN_TTL, &val_int, &sz);
	if (!rc)
		dbg("NN_TTL: %d\n", val_int);
	else
		nn_print_error("Unable to access NN_TTL");
#endif
}

static void
dump_socket(int sock)
{
	show_generic_options(sock);

	/* TODO: For socket-type-specific options */

	/* TODO: For transport-specific options */
}

int
nanomsg_create_slave_socket(unsigned int recv_timeout)
{
	int sock;

	sock = nn_socket(AF_SP, NN_REQ);
	if (sock < 0) {
		nn_print_error("Unable to create the slave socket");
		assert(sock >= 0);
	}

	if (recv_timeout) {
		/* Set the receiving timeout to prevent from blocking if the peer is
		 * not connected.
		 */
		int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVTIMEO,
				       &recv_timeout, sizeof(recv_timeout));
	        ic_assert(!rc, "Failed to set recv timeout");
	}

	if (ic_util_verbose())
		dump_socket(sock);

	return sock;
}

int
nanomsg_create_master_socket(unsigned int send_timeout)
{
	int sock;

	sock = nn_socket(AF_SP, NN_REP);
	if (sock < 0) {
		nn_print_error("Unable to create the master socket");
		assert(sock >= 0);
	}

	if (send_timeout) {
		/* Set the send timeout to prevent from blocking if the peer is
		 * not connected.
		 */
		int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO,
				       &send_timeout, sizeof(send_timeout));
	        ic_assert(!rc, "Failed to set send timeout");
	}

	if (ic_util_verbose())
		dump_socket(sock);

	return sock;
}

void
nanomsg_destroy_socket(int sock)
{
	int rc;

	rc = nn_close(sock);
	if (!rc)
		return;

	int err = nn_errno();
	if (err == EINTR) {
		err("Unable to close the socket: -%d (%s)\n", err,
		    nn_strerror(err));
		rc = nn_close(sock);
	}

	assert(!rc);
}

int
nanomsg_add_slave_endpoint(int sock, char *url)
{
	int ep;

	ep = nn_bind(sock, url);
	if (ep < 0) {
		nn_print_error("Unable to add the slave endpoint");
		assert(ep >= 0);
	}

	return ep;
}

int
nanomsg_add_master_endpoint(int sock, char *url)
{
	int ep;

	ep = nn_connect(sock, url);
	if (ep >= 0) {
		/* ECONNREFUSED is returned if the master endpoint currently
		 * is not able to be connected. nn_connect() should prevent
		 * from leaking ECONNREFUSED.
		 */
		errno = 0;
	} else {
		nn_print_error("Unable to add the master endpoint");
		assert(ep >= 0);
	}

	return ep;
}

void
nanomsg_delete_endpoint(int sock, int ep)
{
	nn_shutdown(sock, ep);
}

int
nanomsg_send_data(int sock, void *data, unsigned long data_len)
{
	int len;

again:
	len = nn_send(sock, data, data_len, 0);
	if (len != data_len) {
		if (errno == EINTR)
			goto again;

		nn_print_error("Unable to send the expected amount of data");
		return -1;
	}

	return 0;
}

int
nanomsg_send_iov_data(int sock, struct nn_iovec *iov, unsigned int nr_iov)
{
	struct nn_msghdr hdr;

	memset(&hdr, 0, sizeof(hdr));
	hdr.msg_iov = iov;
	hdr.msg_iovlen = nr_iov;

	int err;

	do {
		int len = nn_sendmsg(sock, &hdr, 0);
		if (len >= 0)
			return len;

		err = nn_errno();
	} while (err == EINTR);

	if (err == ETIMEDOUT) {
		dbg("Tx iov timeout\n");
		return -1;
	}

	if (err == EAGAIN) {
		err("Need to tx iov again\n");
		return -1;
	}

	err("Failed to send iov data\n");

	return -1;
}

int
nanomsg_receive_data(int sock, void **data, unsigned long *data_len)
{
	if (data && *data && !data_len)
		return -1;

	int rc;
	void *buf;

again:
	if (!data || !*data)
		rc = nn_recv(sock, &buf, NN_MSG, 0);
	else
		rc = nn_recv(sock, *data, *data_len, 0);

	if (rc < 0) {
		if (errno == EINTR)
			goto again;

		nn_print_error("Failed to receive data");
		return -1;
	}

	unsigned long len = rc;

	if (data && !*data)
		*data = buf;

	if (data_len) {
		if (*data_len && (len != *data_len)) {
			nn_print_error("Short of received data");
			err("%ld-byte received, but %ld-byte expected\n", len,
			    *data_len);
			return -1;
		}

		*data_len = len;
	}

	return 0;
}

int
nanomsg_receive_iov_data(int sock, struct nn_iovec *iov,
			 unsigned int nr_iov)
{
	struct nn_msghdr hdr;

	memset(&hdr, 0, sizeof(hdr));
	hdr.msg_iov = iov;
	hdr.msg_iovlen = nr_iov;

	int err;

	do {
		int len = nn_recvmsg(sock, &hdr, 0);
		if (len >= 0)
			return len;

		err = nn_errno();
	} while (err == EINTR);

	if (err == ETIMEDOUT) {
		dbg("Rx iov timeout\n");
		return -1;
	}

	if (err == EAGAIN) {
		err("Need to rx iov again\n");
		return -1;
	}

	err("Failed to receive iov data\n");

	return -1;
}

int
nanomsg_pollin(int *sock, unsigned int nr_sock)
{
	if (!sock || !nr_sock)
		return -1;

	struct nn_pollfd pfd[nr_sock];
	int rc;
	unsigned int i;

	for (i = 0; i < nr_sock; ++i) {
		pfd[i].fd = sock[i];
		pfd[i].events = NN_POLLIN;
		pfd[i].revents = 0;
	}

again:
	rc = nn_poll(pfd, nr_sock, 0);
	if (rc < 0) {
		if (errno == EINTR)
			goto again;

		return -1;
	}

	for (i = 0; i < nr_sock; ++i) {
		if (pfd[i].revents & NN_POLLIN)
			return pfd[i].fd;
	}

	ic_assert(0, "Invalid nn_poll() returned");

	return -1;
}

void *
nanomsg_alloc_data(unsigned long data_len)
{
	return nn_allocmsg(data_len, 0);
}

void
nanomsg_free_data(void *data)
{
	nn_freemsg(data);
}
