// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_basic_config.c -- example usage for part of the pmemkv config API,
 *		which should be preferred. E.g. `put_size(...)` is less error-prone
 *		than `put_int64("size", ...)` and hence recommended.
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

	/* Create config. */
	pmemkv_config *config = pmemkv_config_new();
	ASSERT(config != NULL);

	/* Add path parameter to config. Value is dependent on chosen engine.
	 * E.g. if config is used for cmap engine, it is a path to a database file or
	 * a poolset file. However for vcmap it is a path to an existing directory.
	 * For details see documentation of selected engine. */
	int status = pmemkv_config_put_path(config, argv[1]);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Specify size of the database (to create). */
	status = pmemkv_config_put_size(config, SIZE);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Specify value of create_if_missing flag.
	 * Alternatively, another flag - 'create_or_error_if_exists' can be set using:
	 * `pmemkv_config_put_create_or_error_if_exists`
	 * For differences between the two, see manpage libpmemkv(7). */
	status = pmemkv_config_put_create_if_missing(config, true);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Specify comparator used by the (sorted) engine. */
	pmemkv_comparator *cmp =
		pmemkv_comparator_new(&key_length_compare, "key_length_compare", NULL);
	ASSERT(cmp != NULL);
	status = pmemkv_config_put_comparator(config, cmp);
	ASSERT(status == PMEMKV_STATUS_OK);

	/* Add pointer to OID to the config. For details see libpmemkv(7) manpage. */
	PMEMoid oid;
	status = pmemkv_config_put_oid(config, &oid);
	ASSERT(status == PMEMKV_STATUS_OK);

	pmemkv_config_delete(config);

	return 0;
}
