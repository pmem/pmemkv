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

#include "../../src/engines-experimental/stree.h"
#include "../../src/libpmemkv.hpp"
#include "gtest/gtest.h"

using namespace pmem::kv;

extern std::string test_path;
const size_t SIZE = 1024ull * 1024ull * 512ull;
const size_t LARGE_SIZE = 1024ull * 1024ull * 1024ull * 2ull;

template <size_t POOL_SIZE>
class STreeBaseTest : public testing::Test {
public:
	std::string PATH = test_path + "/stree_test";
	db *kv;

	STreeBaseTest()
	{
		std::remove(PATH.c_str());
		Start(true);
	}

	~STreeBaseTest()
	{
		kv->close();
		delete kv;
		std::remove(PATH.c_str());
	}
	void Restart()
	{
		kv->close();
		delete kv;
		Start(false);
	}

protected:
	void Start(bool create)
	{
		config cfg;

		auto cfg_s = cfg.put_string("path", PATH);

		if (cfg_s != status::OK)
			throw std::runtime_error("putting 'path' to config failed");

		if (create) {
			cfg_s = cfg.put_uint64("force_create", 1);
			if (cfg_s != status::OK)
				throw std::runtime_error(
					"putting 'force_create' to config failed");

			cfg_s = cfg.put_int64("size", POOL_SIZE);

			if (cfg_s != status::OK)
				throw std::runtime_error(
					"putting 'size' to config failed");
		}

		kv = new db;
		auto s = kv->open("stree", std::move(cfg));
		if (s != status::OK)
			throw std::runtime_error(errormsg());
	}
};

typedef STreeBaseTest<SIZE> STreeTest;
typedef STreeBaseTest<LARGE_SIZE> STreeLargeTest;

TEST_F(STreeTest, SimpleTest)
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
	ASSERT_TRUE(kv->defrag() == status::NOT_SUPPORTED);
}

TEST_F(STreeTest, BinaryKeyTest)
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

TEST_F(STreeTest, BinaryValueTest)
{
	std::string value("A\0B\0\0C", 6);
	ASSERT_TRUE(kv->put("key1", value) == status::OK) << errormsg();
	std::string value_out;
	ASSERT_TRUE(kv->get("key1", &value_out) == status::OK &&
		    (value_out.length() == 6) && (value_out == value));
}

TEST_F(STreeTest, EmptyKeyTest)
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

TEST_F(STreeTest, EmptyValueTest)
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

TEST_F(STreeTest, GetClearExternalValueTest_TRACERS_MPHD)
{
	ASSERT_TRUE(kv->put("key1", "cool") == status::OK) << errormsg();
	std::string value = "super";
	ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "cool");

	value = "super";
	ASSERT_TRUE(kv->get("non_existent_key", &value) == status::NOT_FOUND &&
		    value == "super");
}

TEST_F(STreeTest, GetHeadlessTest)
{
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("waldo"));
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(STreeTest, GetMultipleTest)
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

TEST_F(STreeTest, GetMultiple2Test)
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

TEST_F(STreeTest, GetNonexistentTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("waldo"));
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(STreeTest, PutTest)
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

TEST_F(STreeTest, PutKeysOfDifferentSizesTest)
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

TEST_F(STreeTest, PutValuesOfDifferentSizesTest)
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

TEST_F(STreeTest, RemoveAllTest)
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

TEST_F(STreeTest, RemoveAndInsertTest)
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

TEST_F(STreeTest, RemoveExistingTest)
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

TEST_F(STreeTest, RemoveHeadlessTest)
{
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
}

TEST_F(STreeTest, RemoveNonexistentTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
	ASSERT_TRUE(status::OK == kv->exists("key1"));
}

TEST_F(STreeTest, UsesGetAllTest)
{
	ASSERT_TRUE(kv->put("1", "2") == status::OK) << errormsg();
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 1);
	ASSERT_TRUE(kv->put("RR", "记!") == status::OK) << errormsg();
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);

	std::string result;
	kv->get_all(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<1>,<2>|<RR>,<记!>|");
}

TEST_F(STreeTest, UsesGetAboveTest)
{
	ASSERT_TRUE(kv->put("aaa", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("bbb", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ccc", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("rrr", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("sss", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ttt", "6") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("yyy", "记!") == status::OK) << errormsg();

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_above("ccc", cnt) == status::OK);
	ASSERT_EQ(4, cnt);
	std::string result;
	kv->get_above(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_above("a", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	result.clear();
	kv->get_above(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_above("ddd", cnt) == status::OK);
	ASSERT_EQ(4, cnt);
	result.clear();
	kv->get_above(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_above("z", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	result.clear();
	kv->get_above(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());
}

TEST_F(STreeTest, UsesGetEqualAboveTest)
{
	ASSERT_TRUE(kv->put("aaa", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("bbb", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ccc", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("rrr", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("sss", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ttt", "6") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("yyy", "记!") == status::OK) << errormsg();

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	std::string result;
	kv->get_equal_above(
		"",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("ccc", cnt) == status::OK);
	ASSERT_EQ(5, cnt);
	result.clear();
	kv->get_equal_above(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("a", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	result.clear();
	kv->get_equal_above(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("ddd", cnt) == status::OK);
	ASSERT_EQ(4, cnt);
	result.clear();
	kv->get_equal_above(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("x", cnt) == status::OK);
	ASSERT_EQ(1, cnt);
	result.clear();
	kv->get_equal_above(
		"x",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("yyy", cnt) == status::OK);
	ASSERT_EQ(1, cnt);
	result.clear();
	kv->get_equal_above(
		"yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_above("z", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	result.clear();
	kv->get_equal_above(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());
}

TEST_F(STreeTest, UsesGetEqualBelowTest)
{
	ASSERT_TRUE(kv->put("aaa", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("bbb", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ccc", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("rrr", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("sss", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ttt", "6") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("yyy", "记!") == status::OK) << errormsg();

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("yyy", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	std::string result;
	kv->get_equal_below(
		"yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("ttt", cnt) == status::OK);
	ASSERT_EQ(6, cnt);
	result.clear();
	kv->get_equal_below(
		"ttt",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result ==
		    "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("ccc", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	result.clear();
	kv->get_equal_below(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("z", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	result.clear();
	kv->get_equal_below(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("ddd", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	result.clear();
	kv->get_equal_below(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("a", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	result.clear();
	kv->get_equal_below(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_equal_below("", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	result.clear();
	kv->get_equal_below(
		"",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());
}

TEST_F(STreeTest, UsesGetBelowTest)
{
	ASSERT_TRUE(kv->put("aaa", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("bbb", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ccc", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("rrr", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("sss", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ttt", "6") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("yyy", "记!") == status::OK) << errormsg();

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("a", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	std::string result;
	kv->get_below(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("aaa", cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	result.clear();
	kv->get_below(
		"aaa",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("ccc", cnt) == status::OK);
	ASSERT_TRUE(cnt == 2);
	result.clear();
	kv->get_below(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<aaa>,<1>|<bbb>,<2>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("ddd", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	result.clear();
	kv->get_below(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("x", cnt) == status::OK);
	ASSERT_EQ(6, cnt);
	result.clear();
	kv->get_below(
		"x",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result ==
		    "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("yyy", cnt) == status::OK);
	ASSERT_EQ(6, cnt);
	result.clear();
	kv->get_below(
		"yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result ==
		    "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_below("z", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	result.clear();
	kv->get_below(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
}

TEST_F(STreeTest, UsesGetBetweenTest)
{
	ASSERT_TRUE(kv->put("aaa", "1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("bbb", "2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ccc", "3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("rrr", "4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("sss", "5") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("ttt", "6") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("yyy", "记!") == status::OK) << errormsg();

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("", "rrr", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	std::string result;
	kv->get_between(
		"", "rrr",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("ccc", "ttt", cnt) == status::OK);
	ASSERT_EQ(2, cnt);
	result.clear();
	kv->get_between(
		"ccc", "ttt",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<rrr>,<4>|<sss>,<5>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("ddd", "x", cnt) == status::OK);
	ASSERT_EQ(3, cnt);
	result.clear();
	kv->get_between(
		"ddd", "x",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("aaa", "yyy", cnt) == status::OK);
	ASSERT_EQ(5, cnt);
	result.clear();
	kv->get_between(
		"aaa", "yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result == "<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("yyy", "zzz", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	result.clear();
	kv->get_between(
		"yyy", "zzz",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("", "zzz", cnt) == status::OK);
	ASSERT_EQ(7, cnt);
	result.clear();
	kv->get_between(
		"", "zzz",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_between("", "", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	result.clear();
	kv->get_between(
		"", "",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	ASSERT_TRUE(result.empty());
}

// =============================================================================================
// TEST RECOVERY OF SINGLE-LEAF TREE
// =============================================================================================

TEST_F(STreeTest, GetHeadlessAfterRecoveryTest)
{
	Restart();
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(STreeTest, GetMultipleAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("abc", "A1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("def", "B2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("hij", "C3") == status::OK) << errormsg();
	Restart();
	ASSERT_TRUE(kv->put("jkl", "D4") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("mno", "E5") == status::OK) << errormsg();
	std::string value1;
	ASSERT_TRUE(kv->get("abc", &value1) == status::OK && value1 == "A1");
	std::string value2;
	ASSERT_TRUE(kv->get("def", &value2) == status::OK && value2 == "B2");
	std::string value3;
	ASSERT_TRUE(kv->get("hij", &value3) == status::OK && value3 == "C3");
	std::string value4;
	ASSERT_TRUE(kv->get("jkl", &value4) == status::OK && value4 == "D4");
	std::string value5;
	ASSERT_TRUE(kv->get("mno", &value5) == status::OK && value5 == "E5");
}

TEST_F(STreeTest, GetMultiple2AfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("key2", "value2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("key3", "value3") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("key2") == status::OK);
	ASSERT_TRUE(kv->put("key3", "VALUE3") == status::OK) << errormsg();
	Restart();
	std::string value1;
	ASSERT_TRUE(kv->get("key1", &value1) == status::OK && value1 == "value1");
	std::string value2;
	ASSERT_TRUE(kv->get("key2", &value2) == status::NOT_FOUND);
	std::string value3;
	ASSERT_TRUE(kv->get("key3", &value3) == status::OK && value3 == "VALUE3");
}

TEST_F(STreeTest, GetNonexistentAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	Restart();
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(STreeTest, PutAfterRecoveryTest)
{
	std::string value;
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "value1");

	std::string new_value;
	ASSERT_TRUE(kv->put("key1", "VALUE1") == status::OK) << errormsg(); // same size
	ASSERT_TRUE(kv->get("key1", &new_value) == status::OK && new_value == "VALUE1");
	Restart();

	std::string new_value2;
	ASSERT_TRUE(kv->put("key1", "new_value") == status::OK)
		<< errormsg(); // longer size
	ASSERT_TRUE(kv->get("key1", &new_value2) == status::OK &&
		    new_value2 == "new_value");

	std::string new_value3;
	ASSERT_TRUE(kv->put("key1", "?") == status::OK) << errormsg(); // shorter size
	ASSERT_TRUE(kv->get("key1", &new_value3) == status::OK && new_value3 == "?");
}

TEST_F(STreeTest, RemoveAllAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << errormsg();
	Restart();
	ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
}

TEST_F(STreeTest, RemoveAndInsertAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << errormsg();
	Restart();
	ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::OK && value == "tmpvalue1");
	ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
}

TEST_F(STreeTest, RemoveExistingAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("tmpkey2", "tmpvalue2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
	Restart();
	ASSERT_TRUE(kv->remove("tmpkey1") == status::NOT_FOUND);
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->get("tmpkey2", &value) == status::OK && value == "tmpvalue2");
}

TEST_F(STreeTest, RemoveHeadlessAfterRecoveryTest)
{
	Restart();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
}

TEST_F(STreeTest, RemoveNonexistentAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	Restart();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
}

// =============================================================================================
// TEST TREE WITH SINGLE INNER NODE
// =============================================================================================

const size_t INNER_ENTRIES = internal::stree::DEGREE - 1;
const size_t LEAF_ENTRIES = internal::stree::DEGREE - 1;
const size_t SINGLE_INNER_LIMIT = LEAF_ENTRIES * (INNER_ENTRIES - 1);

TEST_F(STreeTest, SingleInnerNodeAscendingTest)
{
	for (std::size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	for (std::size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeAscendingTest2)
{
	for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingTest)
{
	for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingTest2)
{
	for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

// =============================================================================================
// TEST RECOVERY OF TREE WITH SINGLE INNER NODE
// =============================================================================================

TEST_F(STreeTest, SingleInnerNodeAscendingAfterRecoveryTest)
{
	for (size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
	}
	Restart();
	for (size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeAscendingAfterRecoveryTest2)
{
	for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
	}
	Restart();
	for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingAfterRecoveryTest)
{
	for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
	}
	Restart();
	for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingAfterRecoveryTest2)
{
	for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, istr) == status::OK) << errormsg();
	}
	Restart();
	for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == SINGLE_INNER_LIMIT);
}

// =============================================================================================
// TEST LARGE TREE
// =============================================================================================

const int LARGE_LIMIT = 4000000;

TEST_F(STreeLargeTest, LargeAscendingTest)
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

TEST_F(STreeLargeTest, LargeDescendingTest)
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

// =============================================================================================
// TEST RECOVERY OF LARGE TREE
// =============================================================================================

TEST_F(STreeLargeTest, LargeAscendingAfterRecoveryTest)
{
	for (int i = 1; i <= LARGE_LIMIT; i++) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, (istr + "!")) == status::OK) << errormsg();
	}
	Restart();
	for (int i = 1; i <= LARGE_LIMIT; i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == (istr + "!"));
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv->count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == LARGE_LIMIT);
}

TEST_F(STreeLargeTest, LargeDescendingAfterRecoveryTest)
{
	for (int i = LARGE_LIMIT; i >= 1; i--) {
		std::string istr = std::to_string(i);
		ASSERT_TRUE(kv->put(istr, ("ABC" + istr)) == status::OK) << errormsg();
	}
	Restart();
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
