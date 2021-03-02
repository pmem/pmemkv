// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/*
 * pmemkv_iterator.c -- example usage of pmemkv's iterator.
 */

#include "libpmemkv.h"

#include <assert.h>
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

static const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	const size_t n_elements = 10;
	char buffer[64];

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

	LOG("Opening pmemkv database with 'radix' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("radix", cfg, &db);
	ASSERT(s == PMEMKV_STATUS_OK);
	ASSERT(db != NULL);

	LOG("Putting new keys");
	for (size_t i = 0; i < n_elements; ++i) {
		char key[10];
		const char *value = "value";
		sprintf(key, "key%zu", i);
		s = pmemkv_put(db, key, strlen(key), value, strlen(value));
		ASSERT(s == PMEMKV_STATUS_OK);
	}

	/* get a new read iterator */
	pmemkv_iterator *it;
	s = pmemkv_iterator_new(db, &it);
	ASSERT(s == PMEMKV_STATUS_OK);

	LOG("Iterate from first to last element");
	s = pmemkv_iterator_seek_to_first(it);
	ASSERT(s == PMEMKV_STATUS_OK);

	size_t element_number = 0;
	do {
		const char *str;
		size_t cnt;
		/* read a key */
		s = pmemkv_iterator_key(it, &str, &cnt);
		ASSERT(s == PMEMKV_STATUS_OK);
		sprintf(buffer, "Key %zu = %s", element_number++, str);
		LOG(buffer);
	} while (pmemkv_iterator_next(it) != PMEMKV_STATUS_NOT_FOUND);

	/* iterator must be deleted manually */
	pmemkv_iterator_delete(it);

	/* get a new write_iterator */
	pmemkv_write_iterator *w_it;
	s = pmemkv_write_iterator_new(db, &w_it);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* if you want to get a pmemkv_iterator (read iterator) from a
	 * pmemkv_write_iterator, you should do: write_it->iter */
	s = pmemkv_iterator_seek_to_last(w_it->iter);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* get a write range, to modify last element's value */
	char *data;
	size_t cnt;
	s = pmemkv_write_iterator_write_range(w_it, 0, 5, &data, &cnt);
	ASSERT(s == PMEMKV_STATUS_OK);

	for (size_t i = 0; i < cnt; ++i)
		data[i] = 'x';

	/* commit changes */
	s = pmemkv_write_iterator_commit(w_it);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* get a read range, to read modified value */
	const char *str;
	s = pmemkv_iterator_read_range(w_it->iter, 0, 5, &str, &cnt);
	ASSERT(s == PMEMKV_STATUS_OK);

	/* verify a modified value */
	ASSERT(strcmp(str, "xxxxx") == 0);
	sprintf(buffer, "Modified value = %s", str);
	LOG(buffer);

	/* iterator must be deleted manually */
	pmemkv_write_iterator_delete(w_it);

	LOG("Closing database");
	pmemkv_close(db);

	return 0;
}
