/*
 * Copyright 2017-2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../../src/libpmemkv.hpp"
#include "gtest/gtest.h"
#include <sys/stat.h>

#include <cstdio>
#include <memory>

using namespace pmem::kv;

extern std::string test_path;
const size_t SIZE = 1024ull * 1024ull * 512ull;
const size_t LARGE_SIZE = 1024ull * 1024ull * 1024ull * 2ull;

template <size_t POOL_SIZE>
class VSMapBaseTest : public testing::Test {
private:
	std::string PATH = test_path + "/vsmap_test";

public:
	std::unique_ptr<db> kv = nullptr;

	VSMapBaseTest()
	{
		mkdir(PATH.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		config cfg;

		auto cfg_s = cfg.put_string("path", PATH);

		if (cfg_s != status::OK)
			throw std::runtime_error("putting 'path' to config failed");

		cfg_s = cfg.put_int64("size", POOL_SIZE);

		if (cfg_s != status::OK)
			throw std::runtime_error("putting 'size' to config failed");

		kv.reset(new db);
		auto s = kv->open("vsmap", std::move(cfg));
		if (s != status::OK)
			throw std::runtime_error(errormsg());
	}

	~VSMapBaseTest()
	{
		kv->close();
		std::remove(PATH.c_str());
	}
};

using VSMapTest = VSMapBaseTest<SIZE>;
using VSMapLargeTest = VSMapBaseTest<LARGE_SIZE>;

// =============================================================================================
// TEST SMALL COLLECTIONS
// =============================================================================================

TEST_F(VSMapTest, SimpleTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("key1"));
	std::string value;
	ASSERT_TRUE(kv->get("key1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(status::OK == kv->exists("key1"));
	ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "value1");
	value = "";
	kv->get("key1", [&](string_view v) { value.append(v.data(), v.size()); });
	ASSERT_TRUE(value == "value1");
	ASSERT_TRUE(kv->defrag() == status::NOT_SUPPORTED);
}

TEST_F(VSMapTest, BinaryKeyTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("a"));
	ASSERT_TRUE(kv->put("a", "should_not_change") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(status::OK == kv->exists("a"));
	std::string key1 = std::string("a\0b", 3);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists(key1));
	ASSERT_TRUE(kv->put(key1, "stuff") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(status::OK == kv->exists("a"));
	ASSERT_TRUE(status::OK == kv->exists(key1));
	std::string value;
	ASSERT_TRUE(kv->get(key1, &value) == status::OK);
	ASSERT_EQ(value, "stuff");
	std::string value2;
	ASSERT_TRUE(kv->get("a", &value2) == status::OK);
	ASSERT_EQ(value2, "should_not_change");
	ASSERT_TRUE(kv->remove(key1) == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(status::OK == kv->exists("a"));
	ASSERT_TRUE(status::NOT_FOUND == kv->exists(key1));
	std::string value3;
	ASSERT_TRUE(kv->get(key1, &value3) == status::NOT_FOUND);
	ASSERT_TRUE(kv->get("a", &value3) == status::OK && value3 == "should_not_change");
}

TEST_F(VSMapTest, BinaryValueTest_TRACERS_M)
{
	std::string value("A\0B\0\0C", 6);
	ASSERT_TRUE(kv->put("key1", value) == status::OK) << errormsg();
	std::string value_out;
	ASSERT_TRUE(kv->get("key1", &value_out) == status::OK &&
		    (value_out.length() == 6) && (value_out == value));
}

TEST_F(VSMapTest, EmptyKeyTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->put("", "empty") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->put(" ", "single-space") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(kv->put("\t\t", "two-tab") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	std::string value1;
	std::string value2;
	std::string value3;
	ASSERT_TRUE(status::OK == kv->exists(""));
	ASSERT_TRUE(kv->get("", &value1) == status::OK && value1 == "empty");
	ASSERT_TRUE(status::OK == kv->exists(" "));
	ASSERT_TRUE(kv->get(" ", &value2) == status::OK && value2 == "single-space");
	ASSERT_TRUE(status::OK == kv->exists("\t\t"));
	ASSERT_TRUE(kv->get("\t\t", &value3) == status::OK && value3 == "two-tab");
}

TEST_F(VSMapTest, EmptyValueTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->put("empty", "") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->put("single-space", " ") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(kv->put("two-tab", "\t\t") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	std::string value1;
	std::string value2;
	std::string value3;
	ASSERT_TRUE(kv->get("empty", &value1) == status::OK && value1 == "");
	ASSERT_TRUE(kv->get("single-space", &value2) == status::OK && value2 == " ");
	ASSERT_TRUE(kv->get("two-tab", &value3) == status::OK && value3 == "\t\t");
}

TEST_F(VSMapTest, GetClearExternalValueTest_TRACERS_MPHD)
{
	ASSERT_TRUE(kv->put("key1", "cool") == status::OK) << errormsg();
	std::string value = "super";
	ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "cool");

	value = "super";
	ASSERT_TRUE(kv->get("non_existent_key", &value) == status::NOT_FOUND &&
		    value == "super");
}

TEST_F(VSMapTest, GetHeadlessTest_TRACERS_M)
{
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("waldo"));
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, GetMultipleTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("abc", "A1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("def", "B2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("hij", "C3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("jkl", "D4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("mno", "E5") == status::OK) << errormsg();
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 5);
	ASSERT_TRUE(status::OK == kv->exists("abc"));
	std::string value1;
	ASSERT_TRUE(kv->get("abc", &value1) == status::OK && value1 == "A1");
	ASSERT_TRUE(status::OK == kv->exists("def"));
	std::string value2;
	ASSERT_TRUE(kv->get("def", &value2) == status::OK && value2 == "B2");
	ASSERT_TRUE(status::OK == kv->exists("hij"));
	std::string value3;
	ASSERT_TRUE(kv->get("hij", &value3) == status::OK && value3 == "C3");
	ASSERT_TRUE(status::OK == kv->exists("jkl"));
	std::string value4;
	ASSERT_TRUE(kv->get("jkl", &value4) == status::OK && value4 == "D4");
	ASSERT_TRUE(status::OK == kv->exists("mno"));
	std::string value5;
	ASSERT_TRUE(kv->get("mno", &value5) == status::OK && value5 == "E5");
}

TEST_F(VSMapTest, GetMultiple2Test_TRACERS_M)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("key2", "value2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("key3", "value3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("key2") == status::OK);
	ASSERT_TRUE(kv->put("key3", "VALUE3") == status::OK) << errormsg();
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	std::string value1;
	ASSERT_TRUE(kv->get("key1", &value1) == status::OK && value1 == "value1");
	std::string value2;
	ASSERT_TRUE(kv->get("key2", &value2) == status::NOT_FOUND);
	std::string value3;
	ASSERT_TRUE(kv->get("key3", &value3) == status::OK && value3 == "VALUE3");
}

TEST_F(VSMapTest, GetNonexistentTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("waldo"));
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, PutTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);

	std::string value;
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "value1");

	std::string new_value;
	ASSERT_TRUE(kv->put("key1", "VALUE1") == status::OK) << errormsg(); // same size
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->get("key1", &new_value) == status::OK && new_value == "VALUE1");

	std::string new_value2;
	ASSERT_TRUE(kv->put("key1", "new_value") == status::OK)
		<< errormsg(); // longer size
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->get("key1", &new_value2) == status::OK &&
		    new_value2 == "new_value");

	std::string new_value3;
	ASSERT_TRUE(kv->put("key1", "?") == status::OK) << errormsg(); // shorter size
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->get("key1", &new_value3) == status::OK && new_value3 == "?");
}

TEST_F(VSMapTest, PutKeysOfDifferentSizesTest_TRACERS_M)
{
	std::string value;
	ASSERT_TRUE(kv->put("123456789ABCDE", "A") == status::OK) << errormsg();
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->get("123456789ABCDE", &value) == status::OK && value == "A");

	std::string value2;
	ASSERT_TRUE(kv->put("123456789ABCDEF", "B") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(kv->get("123456789ABCDEF", &value2) == status::OK && value2 == "B");

	std::string value3;
	ASSERT_TRUE(kv->put("12345678ABCDEFG", "C") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	ASSERT_TRUE(kv->get("12345678ABCDEFG", &value3) == status::OK && value3 == "C");

	std::string value4;
	ASSERT_TRUE(kv->put("123456789", "D") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 4);
	ASSERT_TRUE(kv->get("123456789", &value4) == status::OK && value4 == "D");

	std::string value5;
	ASSERT_TRUE(kv->put("123456789ABCDEFGHI", "E") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 5);
	ASSERT_TRUE(kv->get("123456789ABCDEFGHI", &value5) == status::OK &&
		    value5 == "E");
}

TEST_F(VSMapTest, PutValuesOfDifferentSizesTest_TRACERS_M)
{
	std::string value;
	ASSERT_TRUE(kv->put("A", "123456789ABCDE") == status::OK) << errormsg();
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->get("A", &value) == status::OK && value == "123456789ABCDE");

	std::string value2;
	ASSERT_TRUE(kv->put("B", "123456789ABCDEF") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(kv->get("B", &value2) == status::OK && value2 == "123456789ABCDEF");

	std::string value3;
	ASSERT_TRUE(kv->put("C", "12345678ABCDEFG") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	ASSERT_TRUE(kv->get("C", &value3) == status::OK && value3 == "12345678ABCDEFG");

	std::string value4;
	ASSERT_TRUE(kv->put("D", "123456789") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 4);
	ASSERT_TRUE(kv->get("D", &value4) == status::OK && value4 == "123456789");

	std::string value5;
	ASSERT_TRUE(kv->put("E", "123456789ABCDEFGHI") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 5);
	ASSERT_TRUE(kv->get("E", &value5) == status::OK &&
		    value5 == "123456789ABCDEFGHI");
}

TEST_F(VSMapTest, RemoveAllTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("tmpkey"));
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, RemoveAndInsertTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("tmpkey"));
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(status::OK == kv->exists("tmpkey1"));
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::OK && value == "tmpvalue1");
	ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("tmpkey1"));
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, RemoveExistingTest_TRACERS_M)
{
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->put("tmpkey2", "tmpvalue2") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->remove("tmpkey1") == status::NOT_FOUND);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("tmpkey1"));
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(status::OK == kv->exists("tmpkey2"));
	ASSERT_TRUE(kv->get("tmpkey2", &value) == status::OK && value == "tmpvalue2");
}

TEST_F(VSMapTest, RemoveHeadlessTest_TRACERS_M)
{
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
}

TEST_F(VSMapTest, RemoveNonexistentTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
	ASSERT_TRUE(status::OK == kv->exists("key1"));
}

TEST_F(VSMapTest, UsesCountTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("A", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AB", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AC", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("B", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BB", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BC", "6") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BD", "7") == status::OK) << errormsg();
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 7);

	ASSERT_TRUE(kv->count_above("", cnt) == status::OK);
	ASSERT_TRUE(cnt == 7);
	ASSERT_TRUE(kv->count_above("A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 6);
	ASSERT_TRUE(kv->count_above("B", cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	ASSERT_TRUE(kv->count_above("BC", cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->count_above("BD", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_above("Z", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);

	ASSERT_TRUE(kv->count_below("", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_below("A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_below("B", cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	ASSERT_TRUE(kv->count_below("BD", cnt) == status::OK);
	ASSERT_TRUE(cnt == 6);
	ASSERT_TRUE(kv->count_below("ZZZZZ", cnt) == status::OK);
	ASSERT_TRUE(cnt == 7);

	ASSERT_TRUE(kv->count_between("", "ZZZZ", cnt) == status::OK);
	ASSERT_TRUE(cnt == 7);
	ASSERT_TRUE(kv->count_between("", "A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_between("", "B", cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);
	ASSERT_TRUE(kv->count_between("A", "B", cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	ASSERT_TRUE(kv->count_between("B", "ZZZZ", cnt) == status::OK);
	ASSERT_TRUE(cnt == 3);

	ASSERT_TRUE(kv->count_between("", "", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_between("A", "A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_between("AC", "A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_between("B", "A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_between("BD", "A", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv->count_between("ZZZ", "B", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
}

TEST_F(VSMapTest, UsesGetAllTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("1", "one") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("2", "two") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << errormsg();

	std::string x;
	kv->get_all([&](string_view k, string_view v) {
		x.append("<")
			.append(k.data(), k.size())
			.append(">,<")
			.append(v.data(), v.size())
			.append(">|");
		return 0;
	});
	ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

	x = "";
	kv->get_all([&](string_view k, string_view v) {
		x.append("<")
			.append(std::string(k.data(), k.size()))
			.append(">,<")
			.append(std::string(v.data(), v.size()))
			.append(">|");
		return 0;
	});
	ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

	x = "";
	kv->get_all(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<")
				.append(std::string(k, kb))
				.append(">,<")
				.append(std::string(v, vb))
				.append(">|");
			return 0;
		},
		&x);
	ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");
}

TEST_F(VSMapTest, UsesGetAllAboveTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("A", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AB", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AC", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("B", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BB", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BC", "6") == status::OK) << errormsg();

	std::string x;
	kv->get_above("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "BB,5|BC,6|");

	x = "";
	kv->get_above("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	x = "";
	kv->get_above("ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x.empty());

	x = "";
	kv->get_above("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "BB,5|BC,6|");

	ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << errormsg();
	x = "";
	kv->get_above(
		"B",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	ASSERT_TRUE(x == "BB,5|BC,6|记!,RR|");
}

TEST_F(VSMapTest, UsesGetAllEqualAboveTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("A", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AB", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AC", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("B", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BB", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BC", "6") == status::OK) << errormsg();

	std::string x;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("B", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	kv->get_equal_above("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "B,4|BB,5|BC,6|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("", cnt) == status::OK);
	ASSERT_EQ(6, cnt);
	x = "";
	kv->get_equal_above("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("ZZZ", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	x = "";
	kv->get_equal_above("ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("AZ", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	x = "";
	kv->get_equal_above("AZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "B,4|BB,5|BC,6|");

	ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("B", cnt) == status::OK);
	ASSERT_EQ(4, cnt);
	x = "";
	kv->get_equal_above(
		"B",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	ASSERT_TRUE(x == "B,4|BB,5|BC,6|记!,RR|");
}

TEST_F(VSMapTest, UsesGetAllEqualBelowTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("A", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AB", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AC", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("B", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BB", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BC", "6") == status::OK) << errormsg();

	std::string x;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("B", cnt) == status::OK);
	ASSERT_EQ(4, cnt);
	kv->get_equal_below("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	x = "";
	kv->get_equal_below("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("ZZZ", cnt) == status::OK);
	ASSERT_EQ(6, cnt);
	x = "";
	kv->get_equal_below("ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("AZ", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	x = "";
	kv->get_equal_below("AZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|");

	ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("记!", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	x = "";
	kv->get_equal_below(
		"记!",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

TEST_F(VSMapTest, UsesGetAllBelowTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("A", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AB", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AC", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("B", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BB", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BC", "6") == status::OK) << errormsg();

	std::string x;
	kv->get_below("AC", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|");

	x = "";
	kv->get_below("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x.empty());

	x = "";
	kv->get_below("ZZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	x = "";
	kv->get_below("AC", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|");

	ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << errormsg();
	x = "";
	kv->get_below(
		"\xFF",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

TEST_F(VSMapTest, UsesGetAllBetweenTest_TRACERS_M)
{
	ASSERT_TRUE(kv->put("A", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AB", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("AC", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("B", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BB", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("BC", "6") == status::OK) << errormsg();

	std::string x;
	kv->get_between("A", "B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "AB,2|AC,3|");

	x = "";
	kv->get_between("", "ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	x = "";
	kv->get_between("", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x.empty());

	x = "";
	kv->get_between("", "B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "A,1|AB,2|AC,3|");

	x = "";
	kv->get_between("", "", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv->get_between("A", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv->get_between("AC", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv->get_between("B", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv->get_between("BD", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv->get_between("ZZZ", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x.empty());

	x = "";
	kv->get_between("A", "B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	ASSERT_TRUE(x == "AB,2|AC,3|");

	ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << errormsg();
	x = "";
	kv->get_between(
		"B", "\xFF",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	ASSERT_TRUE(x == "BB,5|BC,6|记!,RR|");
}

// =============================================================================================
// TEST LARGE COLLECTIONS
// =============================================================================================

const int LARGE_LIMIT = 4000000;

TEST_F(VSMapLargeTest, LargeAscendingTest)
{
	for (int i = 1; i <= LARGE_LIMIT; i++) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, (istr + "!")) == status::OK) << errormsg();
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == (istr + "!"));
	}
	for (int i = 1; i <= LARGE_LIMIT; i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == (istr + "!"));
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == LARGE_LIMIT);
}

TEST_F(VSMapLargeTest, LargeDescendingTest)
{
	for (int i = LARGE_LIMIT; i >= 1; i--) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, ("ABC" + istr)) == status::OK) << errormsg();
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK &&
			    value == ("ABC" + istr));
	}
	for (int i = LARGE_LIMIT; i >= 1; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK &&
			    value == ("ABC" + istr));
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == LARGE_LIMIT);
}
