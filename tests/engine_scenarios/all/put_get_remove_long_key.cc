// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void PutKeysOfDifferentSizesTest(pmem::kv::db &kv)
{
	std::string value;
	ASSERT_STATUS(kv.put("123456789ABCDE", "A"), status::OK);
	ASSERT_SIZE(kv, 1);
	ASSERT_STATUS(kv.get("123456789ABCDE", &value), status::OK);
	UT_ASSERT(value == "A");

	std::string value2;
	ASSERT_STATUS(kv.put("123456789ABCDEF", "B"), status::OK);
	ASSERT_SIZE(kv, 2);
	ASSERT_STATUS(kv.get("123456789ABCDEF", &value2), status::OK);
	UT_ASSERT(value2 == "B");

	std::string value3;
	ASSERT_STATUS(kv.put("12345678ABCDEFG", "C"), status::OK);
	ASSERT_SIZE(kv, 3);
	ASSERT_STATUS(kv.get("12345678ABCDEFG", &value3), status::OK);
	UT_ASSERT(value3 == "C");

	std::string value4;
	ASSERT_STATUS(kv.put("123456789", "D"), status::OK);
	ASSERT_SIZE(kv, 4);
	ASSERT_STATUS(kv.get("123456789", &value4), status::OK);
	UT_ASSERT(value4 == "D");

	std::string value5;
	ASSERT_STATUS(kv.put("123456789ABCDEFGHI", "E"), status::OK);
	ASSERT_SIZE(kv, 5);
	ASSERT_STATUS(kv.get("123456789ABCDEFGHI", &value5), status::OK);
	UT_ASSERT(value5 == "E");
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 PutKeysOfDifferentSizesTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
