/*
 * Inter-Container core
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef IC_H
#define IC_H

#include <eee.h>
#include <icmp.h>
#include <ic_error.h>
#include <subcommand.h>
#include <vector.h>
#include <buffer_stream.h>
#include <bcll.h>

#ifndef IC_DEFAULT_LXC_PATH
  #define IC_DEFAULT_LXC_PATH		"/var/lib/lxc"
#endif

#define stringify(x)			#x

#ifndef offsetof
  #define offsetof(type, member)	((unsigned long)&((type *)0)->member)
#endif

#define container_of(ptr, type, member)	({	\
	const __typeof__(((type *)0)->member) *__ptr = (ptr);	\
	(type *)((char *)__ptr - offsetof(type, member));})

#define align_up(x, n)	(((x) + ((n) - 1)) & ~((n) - 1))
#define aligned(x, n)	(!!((x) & ((n) - 1)))

#define ic_assert(condition, fmt, ...)	\
	do {	\
		if (!(condition)) {	\
			err(fmt ": %s\n", ##__VA_ARGS__, strerror(errno)); \
			exit(EXIT_FAILURE);	\
		}	\
	} while (0)

extern char
ic_git_commit[];

extern char
ic_build_machine[];

typedef unsigned long	ic_transport_t;

extern ic_transport_t
ic_transport_create_master(const char *name);

extern ic_transport_t
ic_transport_create_slave(const char *name);

extern void
ic_transport_destroy(ic_transport_t tr);

extern const char *
ic_transport_name(ic_transport_t tr);

extern int
ic_transport_receive_data(ic_transport_t tr, void **data,
			  unsigned long *data_len);

extern int
ic_transport_send_data(ic_transport_t tr, void *data,
		       unsigned long data_len);

extern int
ic_transport_handle_data(ic_transport_t tr,
			 int (*handler)(void *data, unsigned long data_len));

extern int
ic_transport_append_vector_data(ic_transport_t tr, void *data,
				unsigned long data_len);

extern void
ic_transport_free_data(ic_transport_t tr, void *data);

typedef struct vector_struct	vector_t;

extern int
ic_transport_send_vector_data(ic_transport_t tr, vector_t *vec);

extern void __attribute__ ((constructor))
libic_init(void);

extern void __attribute__((destructor))
libic_fini(void);

extern int
ic_util_verbose(void);

extern void
ic_util_set_verbosity(int verbose);

extern char **
ic_util_split_string(char *in, char *delim, unsigned int *nr);

extern int
ic_util_mkdir(const char *dir, mode_t mode);

extern bool
ic_util_file_exists(const char *file_path);

extern int
ic_conf_file_parse(char *conf_file);

extern char *
ic_conf_file_query(const char *fmt, ...);

extern char *
ic_container_name(void);

extern int
ic_garbage_register(void *data, void *callback);

extern int
ic_garbage_call(void);

extern vector_t *
ic_lxc_list_defined_containers(const char *lxcpath);

#endif	/* IC_H */
