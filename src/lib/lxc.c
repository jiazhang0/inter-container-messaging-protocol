/*
 * LXC infrastructure
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

static bool
config_file_exists(const char *lxcpath, const char *conf_name)
{
	/* lxcpath + '/' + conf_name + '/config' + \0 */
	int path_len = strlen(lxcpath) + 1 + strlen(conf_name) + 7 + 1;
	char *path = alloca(path_len);

	int ret = snprintf(path, path_len, "%s/%s/config", lxcpath, conf_name);
	if (ret < 0 || ret >= path_len)
		return -1;

	return ic_util_file_exists(path);
}

static void
set_name(void *ctx, void *arg)
{
	var_vector_desc_t *desc = (var_vector_desc_t *)ctx;
	const char *name = (const char *)arg;

	int len = strlen(name) + 1;
	desc->buf = strndup(name, len);
	if (!desc->buf)
		return;

	desc->len = len;
}

static void
unset_name(void *ctx)
{
	var_vector_desc_t *desc = (var_vector_desc_t *)ctx;

	eee_mfree(desc->buf);
	desc->buf = NULL;
	desc->len = 0;
}

vector_t *
ic_lxc_list_defined_containers(const char *lxcpath)
{

	if (!lxcpath)
		return NULL;

	vector_t *ret_names = vector_create(0, 0, set_name, unset_name,
					    NULL, VECTOR_FLAGS_DEFAULT);
	if (!ret_names)
		return NULL;

	DIR *dir = opendir(lxcpath);
	if (!dir) {
		err("Unable to open %s\n", lxcpath);
		goto err_opendir;
	}

	while (1) {
		struct dirent *dirent = readdir(dir);
		if (!dirent)
			break;

		/* Ignore ., .. and any hidden directory */
		if (!strcmp(dirent->d_name, "..") || !strcmp(dirent->d_name, "."))
			continue;

		if (!config_file_exists(lxcpath, dirent->d_name))
			continue;

		int rc = vector_increase_obj(ret_names, dirent->d_name);
		if (rc)
			goto err_vector_increase_obj;
	}

	closedir(dir);

	return ret_names;

err_vector_increase_obj:
	closedir(dir);

err_opendir:
	vector_destroy(ret_names);

	return NULL;
}
