// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void test_tx_status(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin();
	auto s = tx.get_status();
	UT_ASSERT(s == status::OK || s == status::NOT_SUPPORTED);
	if (s == status::OK) {
		UT_ASSERT(tx.is_ok());
		tx.get_value().abort();
	}
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
