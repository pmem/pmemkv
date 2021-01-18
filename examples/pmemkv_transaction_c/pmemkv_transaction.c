// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/*
 * pmemkv_transaction.c -- example usage of pmemkv transactions.
 */

#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			puts(pmemkv_errormsg());                                         \
		assert(expr);                                                            \
	} while (0)

#define LOG(msg) puts(msg)

/*
 * This example expects a path to already created database pool.
 *
 * To create a pool use one of the following commands.
 *
 * For regular pools use:
 * pmempool create -l -s 1G "pmemkv_radix" obj path_to_a_pool
 *
 * For poolsets use:
 * pmempool create -l "pmemkv_radix" obj ../examples/example.poolset
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s pool\n", argv[0]);
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	ASSERT(cfg != NULL);

	int s = pmemkv_config_put_path(cfg, argv[1]);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Opening pmemkv database with 'radix' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("radix", cfg, &db);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(db != NULL);

	const char *key1 = "key1";
	const char *value1 = "value1";
	const char *key2 = "key2";
	const char *value2 = "value2";
	const char *key3 = "key3";
	const char *value3 = "value3";

	LOG("Putting new key");
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Starting a tx");
	pmemkv_tx *tx;
	s = pmemkv_tx_begin(db, &tx);
	ASSERT(s == PMEMKV_STATUS_OK);

	s = pmemkv_tx_remove(tx, key1, strlen(key1));
	s = pmemkv_tx_put(tx, key2, strlen(key2), value2, strlen(value2));
	s = pmemkv_tx_put(tx, key3, strlen(key3), value3, strlen(value3));

	/* Until transaction is committed, changes are not visible */
	s = pmemkv_exists(db, key1, strlen(key1));
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_exists(db, key2, strlen(key2));
	ASSERT(s == PMEMKV_STATUS_NOT_FOUND);
	s = pmemkv_exists(db, key3, strlen(key3));
	ASSERT(s == PMEMKV_STATUS_NOT_FOUND);

	s = pmemkv_tx_commit(tx);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* Changes are now visible and pmemkv_tx object is destroyed */
	s = pmemkv_exists(db, key1, strlen(key1));
	ASSERT(s == PMEMKV_STATUS_NOT_FOUND);
	s = pmemkv_exists(db, key2, strlen(key2));
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_exists(db, key3, strlen(key3));
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Ending transaction");
	pmemkv_tx_end(tx);

	LOG("Closing database");
	pmemkv_close(db);

	return 0;
}
