/*
 * Inter-Container transport layer abstraction
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
#include "nanomsg.h"

typedef struct {
	int (*create)(unsigned int timeout);
	void (*destroy)(int sock);
	int (*add_endpoint)(int sock, char *url);
	void (*delete_endpoint)(int sock, int ep);
	int (*send_data)(int sock, void *data, unsigned long data_len);
	int (*receive_data)(int sock, void **data, unsigned long *data_len);
	int (*send_iov_data)(int sock, struct nn_iovec *iov,
			     unsigned int nr_iov);
	int (*pollin)(int *sock, unsigned int nr_sock);
	void *(*alloc_data)(unsigned long data_len);
	void (*free_data)(void *data);
} ic_transport_ops_t;

typedef struct {
	char *name;
	int socket;
	int endpoint;
	ic_transport_ops_t *ops;
	bcll_t link;
	vector_t *tx_vec;
} ic_transport_context_t;

static ic_transport_ops_t master_transport_ops = {
	.create = nanomsg_create_master_socket,
	.destroy = nanomsg_destroy_socket,
	.add_endpoint = nanomsg_add_slave_endpoint,
	.delete_endpoint = nanomsg_delete_endpoint,
	.send_data = nanomsg_send_data,
	.receive_data = nanomsg_receive_data,
	.send_iov_data = nanomsg_send_iov_data,
	.pollin = nanomsg_pollin,
	.alloc_data = nanomsg_alloc_data,
	.free_data = nanomsg_free_data,
};

static ic_transport_ops_t slave_transport_ops = {
	.create = nanomsg_create_slave_socket,
	.destroy = nanomsg_destroy_socket,
	.add_endpoint = nanomsg_add_master_endpoint,
	.delete_endpoint = nanomsg_delete_endpoint,
	.send_data = nanomsg_send_data,
	.receive_data = nanomsg_receive_data,
	.send_iov_data = nanomsg_send_iov_data,
	.pollin = nanomsg_pollin,
	.alloc_data = nanomsg_alloc_data,
	.free_data = nanomsg_free_data,
};

static BCLL_DECLARE(transport_list);

static inline ic_transport_context_t *
to_ic_transport_context_t(ic_transport_t tr)
{
	return (ic_transport_context_t *)tr;
}

static inline ic_transport_t
to_ic_transport_t(ic_transport_context_t *ctx)
{
	return (ic_transport_t)ctx;
}

static ic_transport_context_t *
ic_transport_create(const char *name, int master)
{
	if (!name)
		return NULL;

	int name_len = eee_strlen(name) + 1;
	if (!name_len)
		return NULL;

	/* Always create local directory for local channel */
	char path[PATH_MAX];
	const char *name_list[] = { name, "local", NULL };
	for (int i = 0; name_list[i]; ++i) {
		snprintf(path, sizeof(path), ICMP_CHANNEL_PREFIX "%s", name_list[i]);
		if (access(path, W_OK)) {
			if (errno != ENOENT) {
				err("Unable to access " ICMP_CHANNEL_PREFIX
				    "%s: %s\n", name_list[i], strerror(errno));
				return NULL;
			}

			errno = 0;
			if (ic_util_mkdir(path, S_IRWXU)) {
				err("Unable to create " ICMP_CHANNEL_PREFIX
				    "%s: %s\n", name_list[i], strerror(errno));
				return NULL;
			}
		}
	}

	ic_transport_context_t *ctx = eee_malloc(sizeof(*ctx) + name_len);
	if (!ctx)
		return NULL;

	if (master)
		ctx->ops = &master_transport_ops;
	else
		ctx->ops = &slave_transport_ops;

	unsigned int timeout;
	/* Set the send timeout for the master socket */
	if (master)
		timeout = 100;	/* 100ms */
	else
		timeout = 0;

	ctx->socket = ctx->ops->create(timeout);
	snprintf(path, sizeof(path), "ipc://" ICMP_CHANNEL_PREFIX
		 "%s/ocp-channel", name);
	dbg("Adding the endpoint %s ...\n", path);
	ctx->endpoint = ctx->ops->add_endpoint(ctx->socket, path);

	bcll_add(&transport_list, &ctx->link);

	ctx->name = (char *)(ctx + 1);
	eee_strcpy(ctx->name, name);

	return ctx;
}

ic_transport_t
ic_transport_create_master(const char *name)
{
	ic_transport_context_t *ctx;

	ctx = ic_transport_create(name, 1);
	if (ctx)
		return to_ic_transport_t(ctx);

	return 0;
}

ic_transport_t
ic_transport_create_slave(const char *name)
{
	ic_transport_context_t *ctx;

	ctx = ic_transport_create(name, 0);
	if (ctx)
		return to_ic_transport_t(ctx);

	err("Unable to create the slave transport for %s\n",
	    name);

	return 0;
}

void
ic_transport_destroy(ic_transport_t tr)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	ctx->ops->delete_endpoint(ctx->socket, ctx->endpoint);
	ctx->ops->destroy(ctx->socket);

	bcll_del(&ctx->link);

	eee_mfree(ctx);
}

const char *
ic_transport_name(ic_transport_t tr)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	return ctx->name;
}

int
ic_transport_receive_data(ic_transport_t tr, void **data,
			  unsigned long *data_len)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	return ctx->ops->receive_data(ctx->socket, data, data_len);
}

int
ic_transport_handle_data(ic_transport_t tr,
			 int (*handler)(void *data, unsigned long data_len))
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	void *data = NULL;
	unsigned long data_len = 0;
	int rc = ctx->ops->receive_data(ctx->socket, &data, &data_len);
	if (rc < 0)
		return rc;

	if (handler) {
		rc = handler(data, data_len);
		ctx->ops->free_data(data);
	}

	return rc;
}

int
ic_transport_send_data(ic_transport_t tr, void *data,
		       unsigned long data_len)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	return ctx->ops->send_data(ctx->socket, data, data_len);
}

int
ic_transport_append_vector_data(ic_transport_t tr, void *data,
				unsigned long data_len)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);
	int rc;

	if (ctx->tx_vec) {
		rc = vector_expand(ctx->tx_vec, 1, NULL);
		if (rc)
			return rc;
	} else {
		ctx->tx_vec = vector_create(1, sizeof(struct nn_iovec),
					    NULL, NULL, NULL,
					    VECTOR_FLAGS_DEFAULT);
		if (!ctx->tx_vec)
			return -1;
	}

	struct nn_iovec *iov = vector_get_obj(ctx->tx_vec,
					      vector_get_nr_vector(ctx->tx_vec) - 1);
	iov->iov_base = data;
	iov->iov_len = data_len;

	return 0;
}

int
ic_transport_send_vector_data(ic_transport_t tr, vector_t *vec)
{
	if (!vec) {
		ic_set_errno(IC_ERRNO_INVALID_PARAMETER);
		return -1;
	}

	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	unsigned int nr_vec = vector_get_nr_vector(vec);
	struct nn_iovec *iov = eee_malloc(nr_vec * sizeof(struct nn_iovec));
	if (!iov) {
		ic_set_errno(IC_ERRNO_OUT_OF_MEM);
		return -1;
	}

	for (unsigned int i = 0; i < nr_vec; ++i) {
		size_t len = vector_get_obj_len(vec, i);
		void *buf = vector_get_obj(vec, i);

		if (len) {
			iov[i].iov_len = len;
			iov[i].iov_base = buf;
		} else {
			iov[i].iov_len = NN_MSG;
			iov[i].iov_base = &buf;
		}
	}

	int rc = ctx->ops->send_iov_data(ctx->socket, iov, nr_vec);
	eee_mfree(iov);

	return rc;
}

void *
ic_transport_alloc_data(ic_transport_t tr, unsigned long data_len)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	return ctx->ops->alloc_data(data_len);
}

void
ic_transport_free_data(ic_transport_t tr, void *data)
{
	ic_transport_context_t *ctx = to_ic_transport_context_t(tr);

	ctx->ops->free_data(data);
}
