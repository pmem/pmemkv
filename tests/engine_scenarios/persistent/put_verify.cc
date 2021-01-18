// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put(entry_from_string("abc"), entry_from_string("A1")),
		      status::OK);
	ASSERT_STATUS(kv.put(entry_from_string("def"), entry_from_string("B2")),
		      status::OK);
	ASSERT_STATUS(kv.put(entry_from_string("hij"), entry_from_string("C3")),
		      status::OK);
}

static void check(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put(entry_from_string("jkl"), entry_from_string("D4")),
		      status::OK);
	ASSERT_STATUS(kv.put(entry_from_string("mno"), entry_from_string("E5")),
		      status::OK);
	std::string value1;
	ASSERT_STATUS(kv.get(entry_from_string("abc"), &value1), status::OK);
	UT_ASSERT(value1 == entry_from_string("A1"));
	std::string value2;
	ASSERT_STATUS(kv.get(entry_from_string("def"), &value2), status::OK);
	UT_ASSERT(value2 == entry_from_string("B2"));
	std::string value3;
	ASSERT_STATUS(kv.get(entry_from_string("hij"), &value3), status::OK);
	UT_ASSERT(value3 == entry_from_string("C3"));
	std::string value4;
	ASSERT_STATUS(kv.get(entry_from_string("jkl"), &value4), status::OK);
	UT_ASSERT(value4 == entry_from_string("D4"));
	std::string value5;
	ASSERT_STATUS(kv.get(entry_from_string("mno"), &value5), status::OK);
	UT_ASSERT(value5 == entry_from_string("E5"));
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
