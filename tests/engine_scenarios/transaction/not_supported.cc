// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void test_tx_status(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin();
	UT_ASSERT(!tx.is_ok());

	auto s = tx.get_status();
	ASSERT_STATUS(s, status::NOT_SUPPORTED);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2], {test_tx_status});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
