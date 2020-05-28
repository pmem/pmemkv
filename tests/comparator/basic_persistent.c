// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemkv.h>

#include "unittest.h"

#include <string.h>

static int ARG_VALUE = 0xABC;

static int reverse_three_way_compare(const char *key1, size_t keybytes1, const char *key2,
				     size_t keybytes2, void *arg)
{
	UT_ASSERT(*((int *)arg) == ARG_VALUE);

	/* Compare just first bytes */
	return key2[0] - key1[0];
}

static void test_nullptr_name(const char *engine, pmemkv_config *cfg)
{
	pmemkv_comparator *cmp =
		pmemkv_comparator_new(&reverse_three_way_compare, NULL, &ARG_VALUE);
	UT_ASSERTeq(cmp, NULL);

	pmemkv_config_delete(cfg);
}

int main(int argc, char *argv[])
{
	START();

	if (argc < 3)
		UT_FATAL("usage %s: engine config", argv[0]);

	test_nullptr_name(argv[1], C_CONFIG_FROM_JSON(argv[2]));

	return 0;
}
