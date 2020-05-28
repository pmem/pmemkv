// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemkv.h>

#include <string.h>

#include "unittest.h"

static const char *EXPECTED_ERR_MSG =
	"[pmemkv_open] Comparator with name: \"__pmemkv_binary_comparator\" expected";

int compare(const char *k1, size_t kb1, const char *k2, size_t kb2, void *arg)
{
	return 0;
}

static pmemkv_comparator *create_cmp(const char *name)
{
	pmemkv_comparator *cmp = pmemkv_comparator_new(compare, name, NULL);
	UT_ASSERTne(cmp, NULL);

	return cmp;
}

static void insert(const char *engine, pmemkv_config *cfg)
{
	pmemkv_db *db;
	int s = pmemkv_open(engine, cfg, &db);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	s = pmemkv_put(db, "A", 1, "A", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);
	s = pmemkv_put(db, "B", 1, "B", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);
	s = pmemkv_put(db, "C", 1, "C", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);
	s = pmemkv_put(db, "D", 1, "D", 1);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	pmemkv_close(db);
}

static void check_valid(const char *engine, pmemkv_config *cfg)
{
	pmemkv_db *db;
	int s = pmemkv_open(engine, cfg, &db);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	size_t cnt = UINT64_MAX;
	s = pmemkv_count_above(db, "B", 1, &cnt);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);
	UT_ASSERTeq(cnt, 2);

	cnt = UINT64_MAX;
	s = pmemkv_count_below(db, "B", 1, &cnt);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);
	UT_ASSERTeq(cnt, 1);

	pmemkv_close(db);
}

static void check_invalid(const char *engine, pmemkv_config *cfg)
{
	pmemkv_comparator *cmp = create_cmp("invalid_cmp");

	int s = pmemkv_config_put_object(cfg, "comparator", (void *)cmp,
					 (void(*))pmemkv_comparator_delete);
	UT_ASSERTeq(s, PMEMKV_STATUS_OK);

	pmemkv_db *db;
	s = pmemkv_open(engine, cfg, &db);
	UT_ASSERTeq(s, PMEMKV_STATUS_COMPARATOR_MISMATCH);

	const char *err = pmemkv_errormsg();
	UT_ASSERT(strcmp(err, EXPECTED_ERR_MSG) == 0);
}

int main(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config insert/check", argv[0]);

	const char *engine = argv[1];
	const char *json_config = argv[2];
	const char *mode = argv[3];
	if (strcmp(mode, "insert") != 0 && strcmp(mode, "check") != 0)
		UT_FATAL("usage: %s engine json_config insert/check", argv[0]);

	if (strcmp(mode, "insert") == 0) {
		insert(engine, C_CONFIG_FROM_JSON(json_config));
	} else {
		check_valid(engine, C_CONFIG_FROM_JSON(json_config));
		check_invalid(engine, C_CONFIG_FROM_JSON(json_config));
	}

	return 0;
}
