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

#include <ic.h>

static ic_error_t ic_errno = IC_ERRNO_BASE;

void
ic_set_errno(ic_error_t e)
{
	if (e < IC_ERRNO_BASE)
		err("Invalid oc errno 0x%lx\n", e);

	ic_errno = e;
}

ic_error_t
ic_get_errno(void)
{
	return ic_errno;
}

char *
ic_container_name(void)
{
	/* Manage to get the preferential container name from the configuration
	 * file.
	 */
	char *name = ic_conf_file_query(".container_name");
	if (name)
		return strdup(name);

	/* Otherwise, attempt to get the container name from uts */
	size_t name_sz = 16;

	while (1) {
		name = realloc(name, name_sz);
		if (!name) {
			err("Unable to allocate host name\n");
			break;
		}

		int rc = gethostname(name, name_sz);
		if (!rc) {
			/* System container name (container-dom*) doesn't contain
			 * the type prefix.
			 */
			if (!strncmp("container-dom", name, 8))
				strcpy(name, &name[5]);
			return name;
		}

		if (errno == ENAMETOOLONG)
			name_sz *= 2;
		else {
			err("Unable to get host name: %s\n",
			    strerror(errno));
			break;
		}
	}

	/* TODO: check if this is valid LXC container if not essential */

	return NULL;
}
