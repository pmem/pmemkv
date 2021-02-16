// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

/**
 * Tests adding, reading and removing data; basic short, tests. Only for engines with no
 * fixed-size keys.
 */

using namespace pmem::kv;

static void EmptyKeyTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.put("", "empty"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.put(" ", "1-space"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 2);
	ASSERT_STATUS(kv.put("\t\t", "two-tab"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 3);
	std::string value1;
	std::string value2;
	std::string value3;
	ASSERT_STATUS(kv.exists(""), status::OK);
	ASSERT_STATUS(kv.get("", &value1), status::OK);
	UT_ASSERT(value1 == "empty");
	ASSERT_STATUS(kv.exists(" "), status::OK);
	ASSERT_STATUS(kv.get(" ", &value2), status::OK);
	UT_ASSERT(value2 == "1-space");
	ASSERT_STATUS(kv.exists("\t\t"), status::OK);
	ASSERT_STATUS(kv.get("\t\t", &value3), status::OK);
	UT_ASSERT(value3 == "two-tab");
}

static void EmptyValueTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.put("empty", ""), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.put("1-space", " "), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 2);
	ASSERT_STATUS(kv.put("two-tab", "\t\t"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 3);
	std::string value1;
	std::string value2;
	std::string value3;
	ASSERT_STATUS(kv.get("empty", &value1), status::OK);
	UT_ASSERT(value1 == "");
	ASSERT_STATUS(kv.get("1-space", &value2), status::OK);
	UT_ASSERT(value2 == " ");
	ASSERT_STATUS(kv.get("two-tab", &value3), status::OK);
	UT_ASSERT(value3 == "\t\t");
}

static void EmptyKeyAndValueTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	std::string value = "abc";
	ASSERT_STATUS(kv.get("", &value), status::NOT_FOUND);
	UT_ASSERT(value == "abc");

	ASSERT_STATUS(kv.put("", ""), status::OK);
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);

	ASSERT_STATUS(kv.get("", &value), status::OK);
	UT_ASSERT(value == "");
}

static void PutValuesOfDifferentSizesTest(pmem::kv::db &kv)
{
	std::string value;
	ASSERT_STATUS(kv.put("A", "123456789ABCDE"), status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get("A", &value), status::OK);
	UT_ASSERT(value == "123456789ABCDE");

	std::string value2;
	ASSERT_STATUS(kv.put("B", "123456789ABCDEF"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 2);
	ASSERT_STATUS(kv.get("B", &value2), status::OK);
	UT_ASSERT(value2 == "123456789ABCDEF");

	std::string value3;
	ASSERT_STATUS(kv.put("C", "12345678ABCDEFG"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 3);
	ASSERT_STATUS(kv.get("C", &value3), status::OK);
	UT_ASSERT(value3 == "12345678ABCDEFG");

	std::string value4;
	ASSERT_STATUS(kv.put("D", "123456789"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 4);
	ASSERT_STATUS(kv.get("D", &value4), status::OK);
	UT_ASSERT(value4 == "123456789");

	std::string value5;
	ASSERT_STATUS(kv.put("E", "123456789ABCDEFGHI"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 5);
	ASSERT_STATUS(kv.get("E", &value5), status::OK);
	UT_ASSERT(value5 == "123456789ABCDEFGHI");
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 EmptyKeyTest,
				 EmptyValueTest,
				 EmptyKeyAndValueTest,
				 PutValuesOfDifferentSizesTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
