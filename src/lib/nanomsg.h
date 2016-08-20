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

#ifndef __NANOMSG_H__
#define __NANOMSG_H__

#ifdef EEE_IC
  #include <ic.h>
  #define nanomsg_set_errno		ic_set_errno
#endif

#ifndef NANOMSG_ERRNO_BASE
  #error "NANOMSG_ERRNO_BASE isn't defined"
#endif

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/survey.h>

extern int
nanomsg_create_slave_socket(unsigned int timeout);

extern int
nanomsg_create_master_socket(unsigned int timeout);

extern void
nanomsg_destroy_socket(int sock);

extern int
nanomsg_add_master_endpoint(int sock, char *url);

extern int
nanomsg_add_slave_endpoint(int sock, char *url);

extern void
nanomsg_delete_endpoint(int sock, int ep);

extern int
nanomsg_send_data(int sock, void *data, unsigned long data_len);

extern int
nanomsg_send_iov_data(int sock, struct nn_iovec *iov,
		      unsigned int nr_iov);

extern int
nanomsg_pollin(int *sock, unsigned int nr_sock);

extern int
nanomsg_receive_data(int sock, void **data, unsigned long *data_len);

extern void *
nanomsg_alloc_data(unsigned long data_len);

extern void
nanomsg_free_data(void *data);

#endif	/* __NANOMSG_H__ */
