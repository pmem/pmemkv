// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(pmem::kv::db &kv)
{
	std::string key = entry_from_string("key1");

	std::string expected_value = entry_from_string("value1");
	std::string value;
	ASSERT_STATUS(kv.put(key, expected_value), status::OK);
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	UT_ASSERT(value == expected_value);

	expected_value = entry_from_string("VALUE1");
	std::string new_value;
	ASSERT_STATUS(kv.put(key, expected_value), status::OK); // same size
	ASSERT_STATUS(kv.get(key, &new_value), status::OK);
	UT_ASSERT(new_value == expected_value);

	kv.close();
}

static void check(pmem::kv::db &kv)
{
	std::string key = entry_from_string("key1");

	std::string expected_value = entry_from_string("new_val"); // longer size
	std::string new_value2;
	ASSERT_STATUS(kv.put(key, expected_value), status::OK);
	ASSERT_STATUS(kv.get(key, &new_value2), status::OK);
	UT_ASSERT(new_value2 == expected_value);

	expected_value = entry_from_string("?"); // shorter size
	std::string new_value3;
	ASSERT_STATUS(kv.put(key, expected_value), status::OK);
	ASSERT_STATUS(kv.get(key, &new_value3), status::OK);
	UT_ASSERT(new_value3 == expected_value);
}

static void test(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config insert/check", argv[0]);

	std::string mode = argv[3];
	if (mode != "insert" && mode != "check")
		UT_FATAL("usage: %s engine json_config insert/check", argv[0]);

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	if (mode == "insert") {
		insert(kv);
	} else {
		check(kv);
	}

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
