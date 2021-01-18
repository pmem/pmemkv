// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void insert(pmem::kv::db &kv)
{

	ASSERT_STATUS(kv.put(entry_from_string("key1"), entry_from_string("value1")),
		      status::OK);
	ASSERT_STATUS(kv.put(entry_from_string("key2"), entry_from_string("value2")),
		      status::OK);
	ASSERT_STATUS(kv.put(entry_from_string("key3"), entry_from_string("value3")),
		      status::OK);
	ASSERT_STATUS(kv.remove(entry_from_string("key2")), status::OK);
	ASSERT_STATUS(kv.put(entry_from_string("key3"), entry_from_string("VALUE3")),
		      status::OK);
}

static void check(pmem::kv::db &kv)
{

	std::string value1;
	ASSERT_STATUS(kv.get(entry_from_string("key1"), &value1), status::OK);
	UT_ASSERT(value1 == entry_from_string("value1"));
	std::string value2;
	ASSERT_STATUS(kv.get(entry_from_string("key2"), &value2), status::NOT_FOUND);
	std::string value3;
	ASSERT_STATUS(kv.get(entry_from_string("key3"), &value3), status::OK);
	UT_ASSERT(value3 == entry_from_string("VALUE3"));
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
