// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#include "unittest.hpp"

/**
 * Tests deprecated config methods using C++ API
 */

using namespace pmem::kv;

static void deprecated_funcs_test()
{
	/**
	 * TEST: add and read data from config, using deprecated methods.
	 */
	config cfg;

	status s = cfg.put_force_create(true);
	ASSERT_STATUS(s, status::OK);

	uint64_t int_us;
	s = cfg.get_uint64("create_or_error_if_exists", int_us);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(int_us, 1);

	/* Check if not-deprecated function sets the same config field */
	s = cfg.put_create_or_error_if_exists(false);
	UT_ASSERTne(s, status::OK);
}

int main(int argc, char *argv[])
{
	return run_test([&] { deprecated_funcs_test(); });
}
