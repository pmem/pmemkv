// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(const size_t iterations, pmem::kv::db &kv)
{
	for (size_t i = 1; i <= iterations; i++) {
		auto key = entry_from_number(i);
		auto expected_value = entry_from_number(i, "", "!");
		ASSERT_STATUS(kv.put(key, expected_value), status::OK);
		std::string value;
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == expected_value);
	}
}

static void check(const size_t iterations, pmem::kv::db &kv)
{
	for (size_t i = 1; i <= iterations; i++) {
		auto key = entry_from_number(i);
		auto expected_value = entry_from_number(i, "", "!");
		std::string value;
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == expected_value);
	}
	ASSERT_SIZE(kv, iterations);
}

static void test(int argc, char *argv[])
{
	if (argc < 5)
		UT_FATAL("usage: %s engine json_config insert/check iterations", argv[0]);

	std::string mode = argv[3];
	if (mode != "insert" && mode != "check")
		UT_FATAL("usage: %s engine json_config insert/check iterations", argv[0]);

	auto iterations = std::stoull(argv[4]);

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	if (mode == "insert") {
		insert(iterations, kv);
	} else {
		check(iterations, kv);
	}

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
