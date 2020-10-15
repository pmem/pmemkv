// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

/* this elements' count should exceed db size */
const size_t COUNT_EXCEEDING_DB_SIZE = 5000000;

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto cfg = CONFIG_FROM_JSON(argv[2]);
	cfg.reserve(COUNT_EXCEEDING_DB_SIZE);

	pmem::kv::db kv;
	auto s = kv.open(argv[1], std::move(cfg));
	ASSERT_STATUS(s, pmem::kv::status::OUT_OF_MEMORY);

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
