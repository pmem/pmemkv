// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(pmem::kv::db &kv)

{

	std::string value;
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.get("key1", &value) == status::OK && value == "value1");

	std::string new_value;
	UT_ASSERT(kv.put("key1", "VALUE1") == status::OK); // same size
	UT_ASSERT(kv.get("key1", &new_value) == status::OK && new_value == "VALUE1");

	kv.close();
}
static void check(pmem::kv::db &kv)
{

	std::string new_value2;
	UT_ASSERT(kv.put("key1", "new_value") == status::OK); // longer size
	UT_ASSERT(kv.get("key1", &new_value2) == status::OK && new_value2 == "new_value");

	std::string new_value3;
	UT_ASSERT(kv.put("key1", "?") == status::OK); // shorter size
	UT_ASSERT(kv.get("key1", &new_value3) == status::OK && new_value3 == "?");
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
