/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * pmemkv_config.c -- example usage of pmemkv config API
 */

#include <assert.h>
#include <libpmemkv.h>
#include <libpmemkv_json_config.h>
#include <stdlib.h>
#include <string.h>

/* deleter for int pointer */
void free_int_ptr(void *ptr)
{
	free(ptr);
}

int main()
{
	pmemkv_config *config = pmemkv_config_new();
	assert(config != NULL);

	/* Put int64_t value */
	int status = pmemkv_config_put_int64(config, "size", 1073741824);
	assert(status == PMEMKV_STATUS_OK);

	char buffer[] = "ABC";

	/* Put binary data stored in buffer */
	status = pmemkv_config_put_data(config, "binary", buffer, 3);
	assert(status == PMEMKV_STATUS_OK);

	const void *data;
	size_t data_size;

	/* Get pointer to binary data stored in config */
	status = pmemkv_config_get_data(config, "binary", &data, &data_size);
	assert(status == PMEMKV_STATUS_OK);
	assert(data_size == 3);
	assert(((const char *)data)[0] == 'A');

	int *int_ptr = malloc(sizeof(int));
	assert(int_ptr != NULL);
	*int_ptr = 10;

	/* Put pointer to dynamically allocated object, free_int_ptr is called on
	 * pmemkv_config_delete */
	status = pmemkv_config_put_object(config, "int_ptr", int_ptr, &free_int_ptr);
	assert(status == PMEMKV_STATUS_OK);

	int *get_int_ptr;

	/* Get pointer to object stored in config */
	status = pmemkv_config_get_object(config, "int_ptr", (void **)&get_int_ptr);
	assert(status == PMEMKV_STATUS_OK);
	assert(*get_int_ptr == 10);

	pmemkv_config_delete(config);

	pmemkv_config *config_from_json = pmemkv_config_new();
	assert(config_from_json != NULL);

	/* Parse JSON and put all items found into config_from_json */
	status = pmemkv_config_from_json(config_from_json, "{\"path\":\"/dev/shm\",\
		 \"size\":1073741824,\
		 \"subconfig\":{\
			\"size\":1073741824\
			}\
		}");
	assert(status == PMEMKV_STATUS_OK);

	const char *path;
	status = pmemkv_config_get_string(config_from_json, "path", &path);
	assert(status == PMEMKV_STATUS_OK);
	assert(strcmp(path, "/dev/shm") == 0);

	pmemkv_config *subconfig;

	/* Get pointer to nested configuration "subconfig" */
	status = pmemkv_config_get_object(config_from_json, "subconfig",
					  (void **)&subconfig);
	assert(status == PMEMKV_STATUS_OK);

	size_t sub_size;
	status = pmemkv_config_get_uint64(subconfig, "size", &sub_size);
	assert(status == PMEMKV_STATUS_OK);
	assert(sub_size == 1073741824);

	pmemkv_config_delete(config_from_json);

	return 0;
}
