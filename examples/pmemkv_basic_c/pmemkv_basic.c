// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_basic.c -- example usage of pmemkv.
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
#define MAX_VAL_LEN 64

static const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int get_kv_callback(const char *k, size_t kb, const char *value, size_t value_bytes,
		    void *arg)
{
	printf("   visited: %s\n", k);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	ASSERT(cfg != NULL);

	int s = pmemkv_config_put_path(cfg, argv[1]);
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_size(cfg, SIZE);
	ASSERT(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_create_if_missing(cfg, true);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* Alternatively create_or_error_if_exists flag can be set, to fail if file exists
	 * For differences between the two flags, see manpage libpmemkv(7). */
	/* s = pmemkv_config_put_create_or_error_if_exists(cfg, true); */

	LOG("Opening pmemkv database with 'cmap' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("cmap", cfg, &db);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(db != NULL);

	LOG("Putting new key");
	const char *key1 = "key1";
	const char *value1 = "value1";
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	ASSERT(s == PMEMKV_STATUS_OK);

	size_t cnt;
	s = pmemkv_count_all(db, &cnt);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(cnt == 1);

	LOG("Reading key back");
	char val[MAX_VAL_LEN];
	s = pmemkv_get_copy(db, key1, strlen(key1), val, MAX_VAL_LEN, NULL);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(!strcmp(val, "value1"));

	LOG("Iterating existing keys");
	const char *key2 = "key2";
	const char *value2 = "value2";
	const char *key3 = "key3";
	const char *value3 = "value3";
	pmemkv_put(db, key2, strlen(key2), value2, strlen(value2));
	pmemkv_put(db, key3, strlen(key3), value3, strlen(value3));
	pmemkv_get_all(db, &get_kv_callback, NULL);

	LOG("Removing existing key");
	s = pmemkv_remove(db, key1, strlen(key1));
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(pmemkv_exists(db, key1, strlen(key1)) == PMEMKV_STATUS_NOT_FOUND);

	LOG("Defragmenting the database");
	s = pmemkv_defrag(db, 0, 100);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Closing database");
	pmemkv_close(db);

	return 0;
}
