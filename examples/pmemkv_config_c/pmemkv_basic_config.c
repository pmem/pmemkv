// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * pmemkv_config.c -- example usage of pmemkv config API
 */

#include <assert.h>
#include <libpmemkv.h>
#include <libpmemobj/pool_base.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			puts(pmemkv_errormsg());                                         \
		assert(expr);                                                            \
	} while (0)

static const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int key_length_compare(const char *key1, size_t keybytes1, const char *key2,
		       size_t keybytes2, void *arg)
{
	if (keybytes2 < keybytes1)
		return -1;
	else if (keybytes2 > keybytes1)
		return 1;
	else
		return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	/* Create config */
	pmemkv_config *config = pmemkv_config_new();
	ASSERT(config != NULL);

	/* Add path parameter to config. Meaning of this is dependent on chosen engine.
	 *  E.g. if config is used with cmap enine,
	 *  it is path to a database file or to a poolset file. However for
	 *  vcmap it is path to an existing directory */
	int status = pmemkv_config_put_path(config, argv[1]);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* specifies size of the database */
	status = pmemkv_config_put_size(config, SIZE);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Specifies value of force create flag */
	status = pmemkv_config_put_force_create(config, true);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Specifies comparator used by the engine */
	pmemkv_comparator *cmp =
		pmemkv_comparator_new(&key_length_compare, "key_length_compare", NULL);
	ASSERT(cmp != NULL);
	status = pmemkv_config_put_comparator(config, cmp);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Add Pointer to oid (for details see libpmemobj(7)) which points to engine
	 * data to config */
	PMEMoid oid;
	status = pmemkv_config_put_oid(config, &oid);
	ASSERT(status == PMEMKV_STATUS_OK);

	pmemkv_config_delete(config);

	return 0;
}
