// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"

/**
 * Tests adding, reading and removing data; basic short, tests
 */

using namespace pmem::kv;

static void SimpleTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(status::NOT_FOUND == kv.exists("key1"));
	std::string value;
	UT_ASSERT(kv.get("key1", &value) == status::NOT_FOUND);
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(status::OK == kv.exists("key1"));
	UT_ASSERT(kv.get("key1", &value) == status::OK && value == "value1");
	value = "";
	UT_ASSERT(kv.get("key1", [&](string_view v) {
		value.append(v.data(), v.size());
	}) == status::OK);
	UT_ASSERT(value == "value1");
}

static void EmptyKeyTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.put("", "empty") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.put(" ", "1-space") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	UT_ASSERT(kv.put("\t\t", "two-tab") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 3);
	std::string value1;
	std::string value2;
	std::string value3;
	UT_ASSERT(status::OK == kv.exists(""));
	UT_ASSERT(kv.get("", &value1) == status::OK && value1 == "empty");
	UT_ASSERT(status::OK == kv.exists(" "));
	UT_ASSERT(kv.get(" ", &value2) == status::OK && value2 == "1-space");
	UT_ASSERT(status::OK == kv.exists("\t\t"));
	UT_ASSERT(kv.get("\t\t", &value3) == status::OK && value3 == "two-tab");
}

static void EmptyValueTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.put("empty", "") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.put("1-space", " ") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	UT_ASSERT(kv.put("two-tab", "\t\t") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 3);
	std::string value1;
	std::string value2;
	std::string value3;
	UT_ASSERT(kv.get("empty", &value1) == status::OK && value1 == "");
	UT_ASSERT(kv.get("1-space", &value2) == status::OK && value2 == " ");
	UT_ASSERT(kv.get("two-tab", &value3) == status::OK && value3 == "\t\t");
}

static void GetClearExternalValueTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("key1", "cool") == status::OK);
	std::string value = "super";
	UT_ASSERT(kv.get("key1", &value) == status::OK && value == "cool");

	value = "super";
	UT_ASSERT(kv.get("nope", &value) == status::NOT_FOUND && value == "super");
}

static void GetHeadlessTest(pmem::kv::db &kv)
{
	UT_ASSERT(status::NOT_FOUND == kv.exists("waldo"));
	std::string value;
	UT_ASSERT(kv.get("waldo", &value) == status::NOT_FOUND);
}

static void GetMultipleTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("abc", "A1") == status::OK);
	UT_ASSERT(kv.put("def", "B2") == status::OK);
	UT_ASSERT(kv.put("hij", "C3") == status::OK);
	UT_ASSERT(kv.put("jkl", "D4") == status::OK);
	UT_ASSERT(kv.put("mno", "E5") == status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 5);
	UT_ASSERT(status::OK == kv.exists("abc"));
	std::string value1;
	UT_ASSERT(kv.get("abc", &value1) == status::OK && value1 == "A1");
	UT_ASSERT(status::OK == kv.exists("def"));
	std::string value2;
	UT_ASSERT(kv.get("def", &value2) == status::OK && value2 == "B2");
	UT_ASSERT(status::OK == kv.exists("hij"));
	std::string value3;
	UT_ASSERT(kv.get("hij", &value3) == status::OK && value3 == "C3");
	UT_ASSERT(status::OK == kv.exists("jkl"));
	std::string value4;
	UT_ASSERT(kv.get("jkl", &value4) == status::OK && value4 == "D4");
	UT_ASSERT(status::OK == kv.exists("mno"));
	std::string value5;
	UT_ASSERT(kv.get("mno", &value5) == status::OK && value5 == "E5");
}

static void GetMultiple2Test(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.put("key2", "value2") == status::OK);
	UT_ASSERT(kv.put("key3", "value3") == status::OK);
	UT_ASSERT(kv.remove("key2") == status::OK);
	UT_ASSERT(kv.put("key3", "VALUE3") == status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	std::string value1;
	UT_ASSERT(kv.get("key1", &value1) == status::OK && value1 == "value1");
	std::string value2;
	UT_ASSERT(kv.get("key2", &value2) == status::NOT_FOUND);
	std::string value3;
	UT_ASSERT(kv.get("key3", &value3) == status::OK && value3 == "VALUE3");
}

static void GetNonexistentTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(status::NOT_FOUND == kv.exists("waldo"));
	std::string value;
	UT_ASSERT(kv.get("waldo", &value) == status::NOT_FOUND);
}

static void PutTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);

	std::string value;
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.get("key1", &value) == status::OK && value == "value1");

	std::string new_value;
	UT_ASSERT(kv.put("key1", "VALUE1") == status::OK); // same size
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.get("key1", &new_value) == status::OK && new_value == "VALUE1");

	std::string new_value2;
	UT_ASSERT(kv.put("key1", "new_value") == status::OK); // longer size
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.get("key1", &new_value2) == status::OK && new_value2 == "new_value");

	std::string new_value3;
	UT_ASSERT(kv.put("key1", "?") == status::OK); // shorter size
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.get("key1", &new_value3) == status::OK && new_value3 == "?");
}

static void PutValuesOfDifferentSizesTest(pmem::kv::db &kv)
{
	std::string value;
	UT_ASSERT(kv.put("A", "123456789ABCDE") == status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.get("A", &value) == status::OK && value == "123456789ABCDE");

	std::string value2;
	UT_ASSERT(kv.put("B", "123456789ABCDEF") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	UT_ASSERT(kv.get("B", &value2) == status::OK && value2 == "123456789ABCDEF");

	std::string value3;
	UT_ASSERT(kv.put("C", "12345678ABCDEFG") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 3);
	UT_ASSERT(kv.get("C", &value3) == status::OK && value3 == "12345678ABCDEFG");

	std::string value4;
	UT_ASSERT(kv.put("D", "123456789") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 4);
	UT_ASSERT(kv.get("D", &value4) == status::OK && value4 == "123456789");

	std::string value5;
	UT_ASSERT(kv.put("E", "123456789ABCDEFGHI") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 5);
	UT_ASSERT(kv.get("E", &value5) == status::OK && value5 == "123456789ABCDEFGHI");
}

static void RemoveAllTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.put("tmpkey", "tmpvalue1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.remove("tmpkey") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(status::NOT_FOUND == kv.exists("tmpkey"));
	std::string value;
	UT_ASSERT(kv.get("tmpkey", &value) == status::NOT_FOUND);
}

static void RemoveAndInsertTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.put("tmpkey", "tmpvalue1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.remove("tmpkey") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(status::NOT_FOUND == kv.exists("tmpkey"));
	std::string value;
	UT_ASSERT(kv.get("tmpkey", &value) == status::NOT_FOUND);
	UT_ASSERT(kv.put("tmpkey1", "tmpvalue1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(status::OK == kv.exists("tmpkey1"));
	UT_ASSERT(kv.get("tmpkey1", &value) == status::OK && value == "tmpvalue1");
	UT_ASSERT(kv.remove("tmpkey1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(status::NOT_FOUND == kv.exists("tmpkey1"));
	UT_ASSERT(kv.get("tmpkey1", &value) == status::NOT_FOUND);
}

static void RemoveExistingTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.put("tmpkey1", "tmpvalue1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.put("tmpkey2", "tmpvalue2") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	UT_ASSERT(kv.remove("tmpkey1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.remove("tmpkey1") == status::NOT_FOUND);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(status::NOT_FOUND == kv.exists("tmpkey1"));
	std::string value;
	UT_ASSERT(kv.get("tmpkey1", &value) == status::NOT_FOUND);
	UT_ASSERT(status::OK == kv.exists("tmpkey2"));
	UT_ASSERT(kv.get("tmpkey2", &value) == status::OK && value == "tmpvalue2");
}

static void RemoveHeadlessTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.remove("nada") == status::NOT_FOUND);
}

static void RemoveNonexistentTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.remove("nada") == status::NOT_FOUND);
	UT_ASSERT(status::OK == kv.exists("key1"));
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 SimpleTest,
				 EmptyKeyTest,
				 EmptyValueTest,
				 GetClearExternalValueTest,
				 GetHeadlessTest,
				 GetMultipleTest,
				 GetMultiple2Test,
				 GetNonexistentTest,
				 PutTest,
				 PutValuesOfDifferentSizesTest,
				 RemoveAllTest,
				 RemoveAndInsertTest,
				 RemoveExistingTest,
				 RemoveHeadlessTest,
				 RemoveNonexistentTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
