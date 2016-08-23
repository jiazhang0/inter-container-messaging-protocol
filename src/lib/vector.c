/*
 * Fixed and variable vector infrastructure
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#include <vector.h>

static int
init_budget(vector_t *vec, unsigned int  nr_vector, void **budget,
	    unsigned int budget_size, void *ctor_arg)
{
	unsigned int i, start = vec->nr_vector, end = start + nr_vector;

	if (vec->obj_size) {
		/* Allocate all object buffers for the fixed vector if necessary */
		if (vec->flags & VECTOR_FLAGS_ALLOC) {
			for (i = start; i < end; ++i) {
				void *buf = eee_malloc(vec->obj_size);
				if (!buf) {
					while (i) {
						eee_mfree(budget[--i]);
						budget[i] = NULL;
					}

					err("Unable to allocate object buffer (%d-byte)\n",
					    vec->obj_size);
					vector_set_errno(VECTOR_ERRNO_OUT_OF_MEM);
					return -1;
				}

				if (vec->ctor)
					vec->ctor(buf, ctor_arg);
				else if (vec->flags & VECTOR_FLAGS_ZERO_INIT)
					eee_memset(buf, 0, vec->obj_size);

				budget[i] = buf;
			}
		} else
			eee_memset(budget, 0, budget_size);
	} else {
		if (vec->ctor) {
			var_vector_desc_t *desc = (var_vector_desc_t *)budget;
			for (i = start; i < end; ++i)
				vec->ctor(desc + i, ctor_arg);
		} else
			eee_memset(budget, 0, budget_size);
	}

	return 0;
}

int
vector_init(vector_t *vec, unsigned int nr_vector, unsigned int obj_size,
	    void (*ctor)(void *, void *), void (*dtor)(void *), void *ctor_arg,
	    unsigned long flags)
{
	if (!vec) {
		vector_set_errno(VECTOR_ERRNO_INVALID_PARAMETER);
		return -1;
	}

	/* Note that it is allowed to create an empty budget */
	vec->obj_size = obj_size;
	vec->ctor = ctor;
	vec->dtor = dtor;
	vec->ctor_arg = ctor_arg;
	vec->flags = flags;

	int rc = vector_expand(vec, nr_vector, ctor_arg);
	if (rc)
		return rc;

	return 0;
}

vector_t *
vector_create(unsigned int nr_vector, unsigned int obj_size,
	      void (*ctor)(void *, void *), void (*dtor)(void *),
	      void *ctor_arg, unsigned long flags)
{
	vector_t *vec = eee_malloc(sizeof(*vec));
	if (!vec) {
		err("Unable to allocate vector_t (%ld-byte)\n",
		    sizeof(*vec));
		vector_set_errno(VECTOR_ERRNO_OUT_OF_MEM);
		return NULL;
	}

	eee_memset(vec, 0, sizeof(*vec));

	flags |= VECTOR_FLAGS_NEED_DESTROY;
	int rc = vector_init(vec, nr_vector, obj_size, ctor, dtor, ctor_arg,
			     flags);
	if (rc) {
		eee_mfree(vec);
		return NULL;
	}

	return vec;
}

void
vector_destroy(vector_t *vec)
{
	if (!vec)
		return;

	vector_shrink(vec, vec->nr_vector);

	eee_mfree(vec->budget);

	if (vec->flags & VECTOR_FLAGS_NEED_DESTROY)
		eee_mfree(vec);
}

int
vector_expand(vector_t *vec, int nr_vector, void *ctor_arg)
{
	if (!nr_vector)
		return 0;

	if (nr_vector < 0)
		return vector_shrink(vec, -nr_vector);

	if ((int)vec->nr_vector + nr_vector < 0) {
		err("The requested vectors (%d) underflow (%d)\n",
		    nr_vector, vec->nr_vector);
		vector_set_errno(VECTOR_ERRNO_OUT_OF_RANGE);
		return -1;
	}

	unsigned int sz;
	if (vec->obj_size)
		sz = sizeof(void *);
	else
		sz = sizeof(var_vector_desc_t);

	void **budget;
	unsigned int new_budget_size = align_up((vec->nr_vector + nr_vector) * sz, VECTOR_BUDGET_SIZE);
	if (new_budget_size == vec->budget_size) {
		new_budget_size = vec->budget_size;
		budget = vec->budget;
	} else {
		budget = eee_mrealloc(vec->budget, vec->budget_size, new_budget_size);
		if (!budget) {
			err("Unable to allocate new budget (%d-byte) for the new vectors (%d)\n",
			    new_budget_size, nr_vector);
			vector_set_errno(VECTOR_ERRNO_OUT_OF_MEM);
			return -1;
		}
		if (budget != vec->budget)
			vec->budget = NULL;
	}

	if (!ctor_arg)
		ctor_arg = vec->ctor_arg;

	int rc = init_budget(vec, nr_vector, budget, new_budget_size, ctor_arg);
	if (rc) {
		if (new_budget_size != vec->budget_size)
			eee_mfree(budget);
		return rc;
	}

	vec->budget = budget;
	vec->budget_size = new_budget_size;
	vec->nr_vector += nr_vector;

	return 0;
}

int
vector_shrink(vector_t *vec, int nr_vector)
{
	if (!nr_vector)
		return 0;

	if (nr_vector < 0)
		return vector_expand(vec, -nr_vector, NULL);

	if (vec->dtor) {
		for (unsigned int i = vec->nr_vector - nr_vector; i < vec->nr_vector; ++i) {
			if (vec->obj_size) {
				if (vec->budget[i])
					vec->dtor(vec->budget[i]);
			} else
				vec->dtor((var_vector_desc_t *)vec->budget + i);
		}
	}

	/* TODO: free the unused budget */

	vec->nr_vector -= nr_vector;

	return 0;
}

void *
vector_get_obj(vector_t *vec, int index)
{
	if (index >= vec->nr_vector) {
		err("The index (%d) attempting to cross the border (%d)\n",
		    index, vec->nr_vector);
		vector_set_errno(VECTOR_ERRNO_OUT_OF_RANGE);
		return NULL;
	}

	if (index < 0)
		index = vec->nr_vector - 1;

	if (vec->obj_size)
		return vec->budget[index];
	else
		return ((var_vector_desc_t *)vec->budget + index)->buf;
}

long
vector_get_obj_len(vector_t *vec, int index)
{
	if (index >= vec->nr_vector) {
		err("The index (%d) attempting to cross the border (%d)\n",
		    index, vec->nr_vector);
		vector_set_errno(VECTOR_ERRNO_OUT_OF_RANGE);
		return -1;
	}

	if (index < 0)
		index = vec->nr_vector - 1;

	if (vec->obj_size)
		return vec->obj_size;
	else
		return ((var_vector_desc_t *)vec->budget + index)->len;
}

int
vector_set_obj(vector_t *vec, int index, void *buf, unsigned long len)
{
	if (index >= vec->nr_vector) {
		err("The index (%d) attempting to cross the border (%d)\n",
		    index, vec->nr_vector);
		vector_set_errno(VECTOR_ERRNO_OUT_OF_RANGE);
		return -1;
	}

	if (index < 0)
		index = vec->nr_vector - 1;

	void **orig;
	if (vec->obj_size)
		orig = vec->budget + index;
	else
		orig = (void **)((var_vector_desc_t *)vec->budget + index);

	if (vec->dtor)
		vec->dtor((void *)orig);

	if (vec->obj_size) {
		if (vec->flags & VECTOR_FLAGS_ALLOC)
			eee_memcpy(*orig, buf, vec->obj_size);
		else
			*orig = buf;
	} else {
		((var_vector_desc_t *)orig)->buf = buf;
		((var_vector_desc_t *)orig)->len = len;
	}

	return 0;
}

int
vector_increase_obj(vector_t *vec, void *ctor_arg)
{
	if (!vec) {
		vector_set_errno(VECTOR_ERRNO_INVALID_PARAMETER);
		return -1;
	}

	return vector_expand(vec, 1, ctor_arg);
}

int
vector_get_nr_vector(vector_t *vec)
{
	if (!vec) {
		vector_set_errno(VECTOR_ERRNO_INVALID_PARAMETER);
		return -1;
	}

	return (int)vec->nr_vector;
}

int
vector_to_string_array(vector_t *vec, char ***string_array)
{
	if (!vec || !string_array)
		return -1;

	unsigned int nr_vec = vector_get_nr_vector(vec);
	if (!nr_vec)
		return 0;

	if (!*string_array) {
		*string_array = eee_malloc(sizeof(char *) * nr_vec);
		if (!*string_array)
			return -1;
	}

	for (unsigned int i = 0; i < nr_vec; ++i) {
		if (vec->obj_size)
			(*string_array)[i] = vector_get_obj(vec, i);
		else {
			var_vector_desc_t *desc = vector_get_obj(vec, i);
			(*string_array)[i] = strdup(desc->buf);
			if (!(*string_array)[i]) {
				while (i)
					eee_mfree((*string_array)[--i]);

				return -1;
			}
		}
	}

	return nr_vec;
}
