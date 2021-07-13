// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#include "unittest.h"

/**
 * Tests deprecated config methods using C API
 */

static void deprecated_funcs_test()
{
	/**
	 * TEST: add and read data from config, using deprecated functions
	 */
	pmemkv_config *config = pmemkv_config_new();
	UT_ASSERT(config != NULL);

	int ret = pmemkv_config_put_force_create(config, true);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	uint64_t value_uint;
	ret = pmemkv_config_get_uint64(config, "create_or_error_if_exists", &value_uint);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_uint, 1);

	/* Check if not-deprecated function sets the same config field */
	ret = pmemkv_config_put_create_or_error_if_exists(config, false);
	UT_ASSERTne(ret, PMEMKV_STATUS_OK);

	pmemkv_config_delete(config);
	config = NULL;
}

int main(int argc, char *argv[])
{
	START();

	deprecated_funcs_test();
	return 0;
}
