// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_config.c -- example usage of the part of the pmemkv config API
 *		to set and get data based on their data types.
 */

#include <assert.h>
#include <libpmemkv.h>
#include <libpmemkv_json_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			puts(pmemkv_errormsg());                                         \
		assert(expr);                                                            \
	} while (0)

/* deleter for int pointer */
void free_int_ptr(void *ptr)
{
	free(ptr);
}

int main()
{
	pmemkv_config *config = pmemkv_config_new();
	ASSERT(config != NULL);

	/* Put int64_t value */
	int status = pmemkv_config_put_int64(config, "size", 1073741824);
	ASSERT(status == PMEMKV_STATUS_OK);

	char buffer[] = "ABC";

	/* Put binary data stored in buffer */
	status = pmemkv_config_put_data(config, "binary", buffer, 3);
	ASSERT(status == PMEMKV_STATUS_OK);

	const void *data;
	size_t data_size;

	/* Get pointer to binary data stored in config */
	status = pmemkv_config_get_data(config, "binary", &data, &data_size);
	ASSERT(status == PMEMKV_STATUS_OK);
	ASSERT(data_size == 3);
	ASSERT(((const char *)data)[0] == 'A');

	int *int_ptr = malloc(sizeof(int));
	ASSERT(int_ptr != NULL);
	*int_ptr = 10;

	/* Put pointer to dynamically allocated object, free_int_ptr is called on
	 * pmemkv_config_delete */
	status = pmemkv_config_put_object(config, "int_ptr", int_ptr, &free_int_ptr);
	ASSERT(status == PMEMKV_STATUS_OK);

	int *get_int_ptr;

	/* Get pointer to object stored in config */
	status = pmemkv_config_get_object(config, "int_ptr", (void **)&get_int_ptr);
	ASSERT(status == PMEMKV_STATUS_OK);
	ASSERT(*get_int_ptr == 10);

	pmemkv_config_delete(config);

	pmemkv_config *config_from_json = pmemkv_config_new();
	ASSERT(config_from_json != NULL);

	/* Parse JSON and put all items found into config_from_json */
	status = pmemkv_config_from_json(config_from_json, "{\"path\":\"/dev/shm\",\
		 \"size\":1073741824,\
		 \"subconfig\":{\
			\"size\":1073741824\
			}\
		}");
	ASSERT(status == PMEMKV_STATUS_OK);

	const char *path;
	status = pmemkv_config_get_string(config_from_json, "path", &path);
	ASSERT(status == PMEMKV_STATUS_OK);
	ASSERT(strcmp(path, "/dev/shm") == 0);

	pmemkv_config *subconfig;

	/* Get pointer to nested configuration "subconfig" */
	status = pmemkv_config_get_object(config_from_json, "subconfig",
					  (void **)&subconfig);
	ASSERT(status == PMEMKV_STATUS_OK);

	size_t sub_size;
	status = pmemkv_config_get_uint64(subconfig, "size", &sub_size);
	ASSERT(status == PMEMKV_STATUS_OK);
	ASSERT(sub_size == 1073741824);

	pmemkv_config_delete(config_from_json);

	return 0;
}
