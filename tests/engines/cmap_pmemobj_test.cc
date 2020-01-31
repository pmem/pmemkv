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
#include "../../src/pmemobj_engine.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <memory>

using namespace pmem::kv;

extern std::string test_path;
const size_t SIZE = 1024ull * 1024ull * 512ull;

class CMapPmemobjTest : public testing::Test {
private:
	std::string PATH = test_path + "/cmap_pmemobj_test";

public:
	std::unique_ptr<db> kv = nullptr;

	CMapPmemobjTest()
	{
		std::remove(PATH.c_str());
		Start(true);
	}

	~CMapPmemobjTest()
	{
		pmpool.close();

		std::remove(PATH.c_str());
	}
	void Restart()
	{
		kv.reset(nullptr);
		pmpool.close();
		Start(false);
	}

protected:
	void Start(bool create)
	{
		config cfg;

		if (create) {
			auto pop = pmem::obj::pool<Root>::create(
				PATH.c_str(), "CMapPmemobjTest", SIZE, S_IRWXU);
			pmpool = pop;
		} else {
			auto pop = pmem::obj::pool<Root>::open(PATH.c_str(),
							       "CMapPmemobjTest");
			pmpool = pop;
		}

		auto cfg_o = cfg.put_object("oid", &(pmpool.root()->oid), nullptr);
		if (cfg_o != status::OK)
			throw std::runtime_error("putting 'oid' to config failed");

		kv.reset(new db);
		auto s = kv->open("cmap", std::move(cfg));
		if (s != status::OK)
			throw std::runtime_error(errormsg());
	}

	struct Root {
		PMEMoid oid;
	};

	pmem::obj::pool<Root> pmpool;
};

// =============================================================================================
// TEST SMALL COLLECTIONS
// =============================================================================================

TEST_F(CMapPmemobjTest, SimpleTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, BinaryKeyTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, BinaryValueTest)
{
	std::string value("A\0B\0\0C", 6);
	ASSERT_TRUE(kv->put("key1", value) == status::OK) << errormsg();
	std::string value_out;
	ASSERT_TRUE(kv->get("key1", &value_out) == status::OK &&
		    (value_out.length() == 6) && (value_out == value));
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, EmptyKeyTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, EmptyValueTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetClearExternalValueTest)
{
	ASSERT_TRUE(kv->put("key1", "cool") == status::OK) << errormsg();
	std::string value = "super";
	ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "cool");

	value = "super";
	ASSERT_TRUE(kv->get("non_existent_key", &value) == status::NOT_FOUND &&
		    value == "super");
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetHeadlessTest)
{
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("waldo"));
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetMultipleTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetMultiple2Test)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetNonexistentTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(status::NOT_FOUND == kv->exists("waldo"));
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, PutTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, PutKeysOfDifferentSizesTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, PutValuesOfDifferentSizesTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveAllTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveAndInsertTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveExistingTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveHeadlessTest)
{
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveNonexistentTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
	ASSERT_TRUE(status::OK == kv->exists("key1"));
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, UsesGetAllTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

// =============================================================================================
// TEST RECOVERY
// =============================================================================================

TEST_F(CMapPmemobjTest, GetHeadlessAfterRecoveryTest)
{
	Restart();
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetMultipleAfterRecoveryTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetMultiple2AfterRecoveryTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, GetNonexistentAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	Restart();
	std::string value;
	ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, PutAfterRecoveryTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveAllAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << errormsg();
	Restart();
	ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveAndInsertAfterRecoveryTest)
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
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveExistingAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->put("tmpkey2", "tmpvalue2") == status::OK) << errormsg();
	ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
	Restart();
	ASSERT_TRUE(kv->remove("tmpkey1") == status::NOT_FOUND);
	std::string value;
	ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv->get("tmpkey2", &value) == status::OK && value == "tmpvalue2");
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveHeadlessAfterRecoveryTest)
{
	Restart();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, RemoveNonexistentAfterRecoveryTest)
{
	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();
	Restart();
	ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
	ASSERT_TRUE(kv->defrag() == status::OK);
}

TEST_F(CMapPmemobjTest, TransactionTest)
{
	std::string value;
	ASSERT_TRUE(kv->get("key1", &value) == status::NOT_FOUND);

	pmem::obj::transaction::run(pmpool, [&] {
		ASSERT_TRUE(kv->put("key1", "value1") == status::TRANSACTION_SCOPE_ERROR)
			<< errormsg();
	});

	ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << errormsg();

	pmem::obj::transaction::run(pmpool, [&] {
		ASSERT_TRUE(kv->get("key1", &value) == status::TRANSACTION_SCOPE_ERROR)
			<< errormsg();
	});

	value = "";
	ASSERT_TRUE(kv->get("key1", &value) == status::OK) << errormsg();
	ASSERT_TRUE(value == "value1") << errormsg();

	pmem::obj::transaction::run(pmpool, [&] {
		ASSERT_TRUE(kv->remove("key1") == status::TRANSACTION_SCOPE_ERROR)
			<< errormsg();
	});

	ASSERT_TRUE(kv->remove("key1") == status::OK) << errormsg();
	ASSERT_TRUE(kv->defrag() == status::OK);
}
