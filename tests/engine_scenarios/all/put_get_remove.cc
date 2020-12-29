// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"

/**
 * Tests adding, reading and removing data; basic short, tests
 */

using namespace pmem::kv;

static void SimpleTest(pmem::kv::db &kv)
{
	std::string value = "abcdefgh";
	ASSERT_STATUS(kv.put(value, value), status::OK);

	std::string v1 = "";
	ASSERT_STATUS(kv.get(value, &v1), status::OK);

	std::cerr << std::hex << *(uint64_t *)(v1.data()) << std::endl;
	std::cerr << std::hex << *(uint64_t *)(value.data()) << std::endl;

	UT_ASSERT(*(uint64_t *)(v1.data()) == *(uint64_t *)(value.data()));
}

static void EmptyKeyTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	std::cerr << cnt << std::endl;
	UT_ASSERT(cnt == 0);
	std::string to_put = "" + std::string(7, 0);
	ASSERT_STATUS(kv.put(to_put, to_put), status::OK);
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
	ASSERT_STATUS(kv.exists(to_put), status::OK);
	ASSERT_STATUS(kv.get(to_put, &value1), status::OK);
	// UT_ASSERT(value1 == "empty");
	std::cerr << *(uint64_t *)(value1.data()) << std::endl;
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

static void GetClearExternalValueTest(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("key1", "cool"), status::OK);
	std::string value = "super";
	ASSERT_STATUS(kv.get("key1", &value), status::OK);
	UT_ASSERT(value == "cool");

	value = "super";
	ASSERT_STATUS(kv.get("nope", &value), status::NOT_FOUND);
	UT_ASSERT(value == "super");
}

static void GetHeadlessTest(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.exists("waldo"), status::NOT_FOUND);
	std::string value;
	ASSERT_STATUS(kv.get("waldo", &value), status::NOT_FOUND);
}

static void GetMultipleTest(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("abc", "A1"), status::OK);
	ASSERT_STATUS(kv.put("def", "B2"), status::OK);
	ASSERT_STATUS(kv.put("hij", "C3"), status::OK);
	ASSERT_STATUS(kv.put("jkl", "D4"), status::OK);
	ASSERT_STATUS(kv.put("mno", "E5"), status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 5);
	ASSERT_STATUS(kv.exists("abc"), status::OK);
	std::string value1;
	ASSERT_STATUS(kv.get("abc", &value1), status::OK);
	UT_ASSERT(value1 == "A1");
	ASSERT_STATUS(kv.exists("def"), status::OK);
	std::string value2;
	ASSERT_STATUS(kv.get("def", &value2), status::OK);
	UT_ASSERT(value2 == "B2");
	ASSERT_STATUS(kv.exists("hij"), status::OK);
	std::string value3;
	ASSERT_STATUS(kv.get("hij", &value3), status::OK);
	UT_ASSERT(value3 == "C3");
	ASSERT_STATUS(kv.exists("jkl"), status::OK);
	std::string value4;
	ASSERT_STATUS(kv.get("jkl", &value4), status::OK);
	UT_ASSERT(value4 == "D4");
	ASSERT_STATUS(kv.exists("mno"), status::OK);
	std::string value5;
	ASSERT_STATUS(kv.get("mno", &value5), status::OK);
	UT_ASSERT(value5 == "E5");
}

static void GetMultiple2Test(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("key1", "value1"), status::OK);
	ASSERT_STATUS(kv.put("key2", "value2"), status::OK);
	ASSERT_STATUS(kv.put("key3", "value3"), status::OK);
	ASSERT_STATUS(kv.remove("key2"), status::OK);
	ASSERT_STATUS(kv.put("key3", "VALUE3"), status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 2);
	std::string value1;
	ASSERT_STATUS(kv.get("key1", &value1), status::OK);
	UT_ASSERT(value1 == "value1");
	std::string value2;
	ASSERT_STATUS(kv.get("key2", &value2), status::NOT_FOUND);
	std::string value3;
	ASSERT_STATUS(kv.get("key3", &value3), status::OK);
	UT_ASSERT(value3 == "VALUE3");
}

static void GetNonexistentTest(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("key1", "value1"), status::OK);
	ASSERT_STATUS(kv.exists("waldo"), status::NOT_FOUND);
	std::string value;
	ASSERT_STATUS(kv.get("waldo", &value), status::NOT_FOUND);
}

static void PutTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	std::string value;
	ASSERT_STATUS(kv.put("key1", "value1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get("key1", &value), status::OK);
	UT_ASSERT(value == "value1");

	std::string new_value;
	ASSERT_STATUS(kv.put("key1", "VALUE1"), status::OK); // same size
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get("key1", &new_value), status::OK);
	UT_ASSERT(new_value == "VALUE1");

	std::string new_value2;
	ASSERT_STATUS(kv.put("key1", "new_value"), status::OK); // longer size
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get("key1", &new_value2), status::OK);
	UT_ASSERT(new_value2 == "new_value");

	std::string new_value3;
	ASSERT_STATUS(kv.put("key1", "?"), status::OK); // shorter size
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get("key1", &new_value3), status::OK);
	UT_ASSERT(new_value3 == "?");
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

static void RemoveAllTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.put("tmpkey", "tmpvalue1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.remove("tmpkey"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.exists("tmpkey"), status::NOT_FOUND);
	std::string value;
	ASSERT_STATUS(kv.get("tmpkey", &value), status::NOT_FOUND);
}

static void RemoveAndInsertTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.put("tmpkey", "tmpvalue1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.remove("tmpkey"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.exists("tmpkey"), status::NOT_FOUND);
	std::string value;
	ASSERT_STATUS(kv.get("tmpkey", &value), status::NOT_FOUND);
	ASSERT_STATUS(kv.put("tmpkey1", "tmpvalue1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.exists("tmpkey1"), status::OK);
	ASSERT_STATUS(kv.get("tmpkey1", &value), status::OK);
	UT_ASSERT(value == "tmpvalue1");
	ASSERT_STATUS(kv.remove("tmpkey1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.exists("tmpkey1"), status::NOT_FOUND);
	ASSERT_STATUS(kv.get("tmpkey1", &value), status::NOT_FOUND);
}

static void RemoveExistingTest(pmem::kv::db &kv)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.put("tmpkey1", "tmpvalue1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.put("tmpkey2", "tmpvalue2"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 2);
	ASSERT_STATUS(kv.remove("tmpkey1"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.remove("tmpkey1"), status::NOT_FOUND);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.exists("tmpkey1"), status::NOT_FOUND);
	std::string value;
	ASSERT_STATUS(kv.get("tmpkey1", &value), status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("tmpkey2"), status::OK);
	ASSERT_STATUS(kv.get("tmpkey2", &value), status::OK);
	UT_ASSERT(value == "tmpvalue2");
}

static void RemoveHeadlessTest(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.remove("nada"), status::NOT_FOUND);
}

static void RemoveNonexistentTest(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("key1", "value1"), status::OK);
	ASSERT_STATUS(kv.remove("nada"), status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("key1"), status::OK);
}

static void MoveDBTest(pmem::kv::db &kv)
{
	/**
	 * TEST: test db constructor from another instance of db class
	 *	and move assignment operator (from different and the same db).
	 */

	/* put key1 in original db */
	ASSERT_STATUS(kv.put("key1", "value1"), status::OK);

	db kv_new(std::move(kv));

	ASSERT_STATUS(kv_new.put("key2", "value2"), status::OK);
	std::string value;
	value = "ABC";

	ASSERT_STATUS(kv_new.get("key1", &value), status::OK);
	UT_ASSERT(value == "value1");
	ASSERT_STATUS(kv_new.get("key2", &value), status::OK);
	UT_ASSERT(value == "value2");
	ASSERT_STATUS(kv_new.remove("key1"), status::OK);

	db kv_assign_new = std::move(kv_new);
	auto &kv_assign_new2 = kv_assign_new;
	kv_assign_new = std::move(kv_assign_new2);

	ASSERT_STATUS(kv_assign_new.put("key3", "value3"), status::OK);

	ASSERT_STATUS(kv_assign_new.get("key2", &value), status::OK);
	UT_ASSERT(value == "value2");
	ASSERT_STATUS(kv_assign_new.get("key3", &value), status::OK);
	UT_ASSERT(value == "value3");

	ASSERT_STATUS(kv_assign_new.remove("key2"), status::OK);
	ASSERT_STATUS(kv_assign_new.remove("key3"), status::OK);

	kv_assign_new.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(
		argv[1], argv[2],
		{
			SimpleTest,
			EmptyKeyTest,
			EmptyValueTest,
			EmptyKeyAndValueTest,
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
			/* move DB test has to be the last one; it invalidates kv */
			MoveDBTest,
		});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
