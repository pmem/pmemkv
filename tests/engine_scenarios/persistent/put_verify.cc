// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("abc", "A1") == status::OK);
	UT_ASSERT(kv.put("def", "B2") == status::OK);
	UT_ASSERT(kv.put("hij", "C3") == status::OK);
}

static void check(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("jkl", "D4") == status::OK);
	UT_ASSERT(kv.put("mno", "E5") == status::OK);
	std::string value1;
	UT_ASSERT(kv.get("abc", &value1) == status::OK && value1 == "A1");
	std::string value2;
	UT_ASSERT(kv.get("def", &value2) == status::OK && value2 == "B2");
	std::string value3;
	UT_ASSERT(kv.get("hij", &value3) == status::OK && value3 == "C3");
	std::string value4;
	UT_ASSERT(kv.get("jkl", &value4) == status::OK && value4 == "D4");
	std::string value5;
	UT_ASSERT(kv.get("mno", &value5) == status::OK && value5 == "E5");
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
