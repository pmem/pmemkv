// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemkv.h>

#include "unittest.h"

#include <string.h>

static int ARG_VALUE = 0xABC;

int reverse_three_way_compare(const char *key1, size_t keybytes1, const char *key2,
			      size_t keybytes2, void *arg)
{
	UT_ASSERT(*((int *)arg) == ARG_VALUE);

	/* Compare just first bytes */
	return key2[0] - key1[0];
}

static const char *keys[3];
static int keys_count = 0;

int get_callback(const char *key, size_t kb, const char *value, size_t vb, void *arg)
{
	char *key_copy = malloc(kb);
	memcpy(key_copy, key, kb);

	keys[keys_count++] = key_copy;

	return 0;
}

void test_valid_comparator(const char *engine, pmemkv_config *cfg)
{
	pmemkv_comparator *cmp = pmemkv_comparator_new(&ARG_VALUE);
	UT_ASSERTne(cmp, NULL);

	int s = pmemkv_comparator_set_function(cmp, &reverse_three_way_compare);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	s = pmemkv_comparator_set_name(cmp, "single_byte_compare");
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	s = pmemkv_config_put_object(cfg, "comparator", cmp,
				     (void (*)(void *)) & pmemkv_comparator_delete);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	pmemkv_db *db;

	s = pmemkv_open(engine, cfg, &db);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	s = pmemkv_put(db, "123", 3, "1", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	s = pmemkv_put(db, "333", 3, "1", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	s = pmemkv_put(db, "223", 3, "1", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	pmemkv_get_all(db, &get_callback, NULL);

	UT_ASSERTeq(keys_count, 3);
	UT_ASSERTeq(memcmp(keys[0], "333", 3), 0);
	UT_ASSERTeq(memcmp(keys[1], "223", 3), 0);
	UT_ASSERTeq(memcmp(keys[2], "123", 3), 0);

	pmemkv_close(db);
}

void test_nullptr_function(const char *engine, pmemkv_config *cfg)
{
	pmemkv_comparator *cmp = pmemkv_comparator_new(&ARG_VALUE);
	UT_ASSERTne(cmp, NULL);

	int s = pmemkv_comparator_set_function(cmp, NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	pmemkv_comparator_delete(cmp);
	pmemkv_config_delete(cfg);
}

int main(int argc, char *argv[])
{
	START();

	if (argc < 3)
		UT_FATAL("usage %s: engine config", argv[0]);

	test_valid_comparator(argv[1], C_CONFIG_FROM_JSON(argv[2]));

	return 0;
}
