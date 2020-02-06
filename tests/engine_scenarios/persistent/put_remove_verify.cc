// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.put("key2", "value2") == status::OK);
	UT_ASSERT(kv.put("key3", "value3") == status::OK);
	UT_ASSERT(kv.remove("key2") == status::OK);
	UT_ASSERT(kv.put("key3", "VALUE3") == status::OK);
}

static void check(pmem::kv::db &kv)
{

	std::string value1;
	UT_ASSERT(kv.get("key1", &value1) == status::OK && value1 == "value1");
	std::string value2;
	UT_ASSERT(kv.get("key2", &value2) == status::NOT_FOUND);
	std::string value3;
	UT_ASSERT(kv.get("key3", &value3) == status::OK && value3 == "VALUE3");
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
