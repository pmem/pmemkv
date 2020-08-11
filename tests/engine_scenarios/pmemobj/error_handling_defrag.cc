// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static void DefragInvalidArgument(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.defrag(50, 100), pmem::kv::status::INVALID_ARGUMENT);
	ASSERT_STATUS(kv.defrag(0, 101), pmem::kv::status::INVALID_ARGUMENT);
	ASSERT_STATUS(kv.defrag(101, 0), pmem::kv::status::INVALID_ARGUMENT);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	DefragInvalidArgument(kv);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
