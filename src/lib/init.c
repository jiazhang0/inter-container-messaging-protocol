/*
 * Libicmp constructor and destructor
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

static int initialized;

void __attribute__ ((constructor))
libicmp_init(void)
{
	if (initialized)
		return;

	initialized = 1;
}

void __attribute__((destructor))
libicmp_fini(void)
{
}
