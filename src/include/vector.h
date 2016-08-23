/*
 * Vector infrastructure
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef VECTOR_H
#define VECTOR_H

#ifdef EEE_IC
  #include <ic.h>
  #define vector_set_errno			ic_set_errno
#endif

#ifndef VECTOR_ERRNO_BASE
  #error "VECTOR_ERRNO_BASE isn't defined"
#endif

/* Vector-specific errno */
#define VECTOR_ERRNO_INVALID_PARAMETER		(VECTOR_ERRNO_BASE + 0)
#define VECTOR_ERRNO_OUT_OF_MEM			(VECTOR_ERRNO_BASE + 1)
#define VECTOR_ERRNO_OUT_OF_RANGE		(VECTOR_ERRNO_BASE + 2)

/* Allocate all object buffers for the fixed vector */
#define VECTOR_FLAGS_ALLOC			(1 << 0)
/* Zero out each object buffer for the fixed vector */
#define VECTOR_FLAGS_ZERO_INIT			(1 << 1)
#define VECTOR_FLAGS_DTOR_REVERSE_ORDER		(1 << 2)
/* (Internal use only) Free vector itself when calling vector_destory() */
#define VECTOR_FLAGS_NEED_DESTROY		(1 << 24)

#define VECTOR_FLAGS_DEFAULT			VECTOR_FLAGS_ALLOC

#define VECTOR_BUDGET_SIZE			4096

typedef struct {
	void *buf;
	unsigned long len;
} var_vector_desc_t;

typedef struct vector_struct {
	unsigned int nr_vector;
	void **budget;
	unsigned int budget_size;
	unsigned int obj_size;
	void (*ctor)(void *buf, void *arg);
	void (*dtor)(void *buf);
	void *ctor_arg;
	unsigned long flags;
} vector_t;

extern int
vector_init(vector_t *vec, unsigned int nr_vector, unsigned int obj_size,
	    void (*ctor)(void *, void *), void (*dtor)(void *), void *ctor_arg,
	    unsigned long flags);

extern vector_t *
vector_create(unsigned int nr_vector, unsigned int obj_size,
	      void (*ctor)(void *, void *), void (*dtor)(void *),
	      void *ctor_arg, unsigned long flags);

extern void
vector_destroy(vector_t *vec);

extern int
vector_expand(vector_t *vec, int nr_vector, void *ctor_arg);

extern int
vector_shrink(vector_t *vec, int nr_vector);

extern void *
vector_get_obj(vector_t *vec, int index);

extern long
vector_get_obj_len(vector_t *vec, int index);

extern void **
vector_get_obj_ref(vector_t *vec, int index);

extern int
vector_set_obj(vector_t *vec, int index, void *buf,
	       unsigned long len);

extern int
vector_increase_obj(vector_t *vec, void *ctor_arg);

extern int
vector_get_nr_vector(vector_t *vec);

extern int
vector_to_string_array(vector_t *vec, char ***string_array);

#endif	/* VECTOR_H */
