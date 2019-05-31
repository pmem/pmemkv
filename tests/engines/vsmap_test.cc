/*
 * Copyright 2017-2019, Intel Corporation
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

#include "gtest/gtest.h"
#include "../../src/libpmemkv.hpp"
#include <libpmemobj.h>

using namespace pmem::kv;

const std::string PATH = "/dev/shm";
const size_t SIZE = 1024ull * 1024ull * 512ull;
const size_t LARGE_SIZE = 1024ull * 1024ull * 1024ull * 2ull;

template<size_t POOL_SIZE>
class VSMapBaseTest : public testing::Test {
public:
    db* kv;

    VSMapBaseTest() {
        char config[255];
        auto n = sprintf(config, "{\"path\": \"%s\", \"size\" : %lu}", PATH.c_str(), POOL_SIZE);

        if (n < 0)
            throw std::runtime_error("sprintf failed");

        kv = new db("vsmap", config);
    }

    ~VSMapBaseTest() {
        delete kv;
    }
};

using VSMapTest = VSMapBaseTest<SIZE>;
using VSMapLargeTest = VSMapBaseTest<LARGE_SIZE>;

// =============================================================================================
// TEST SMALL COLLECTIONS
// =============================================================================================

TEST_F(VSMapTest, SimpleTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(!kv->exists("key1"));
    std::string value;
    ASSERT_TRUE(kv->get("key1", &value) == status::NOT_FOUND);
    ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->exists("key1"));
    ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "value1");
    value = "";
    kv->get("key1", [&](const char *v, int vb) { value.append(v, vb); });
    ASSERT_TRUE(value == "value1");
    value = "";
    kv->get("key1", [&](const std::string& v) { value.append(v); });
    ASSERT_TRUE(value == "value1");
}

TEST_F(VSMapTest, BinaryKeyTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(!kv->exists("a"));
    ASSERT_TRUE(kv->put("a", "should_not_change") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->exists("a"));
    std::string key1 = std::string("a\0b", 3);
    ASSERT_TRUE(!kv->exists(key1));
    ASSERT_TRUE(kv->put(key1, "stuff") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    ASSERT_TRUE(kv->exists("a"));
    ASSERT_TRUE(kv->exists(key1));
    std::string value;
    ASSERT_TRUE(kv->get(key1, &value) == status::OK);
    ASSERT_EQ(value, "stuff");
    std::string value2;
    ASSERT_TRUE(kv->get("a", &value2) == status::OK);
    ASSERT_EQ(value2, "should_not_change");
    ASSERT_TRUE(kv->remove(key1) == status::OK);
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->exists("a"));
    ASSERT_TRUE(!kv->exists(key1));
    std::string value3;
    ASSERT_TRUE(kv->get(key1, &value3) == status::NOT_FOUND);
    ASSERT_TRUE(kv->get("a", &value3) == status::OK && value3 == "should_not_change");
}

TEST_F(VSMapTest, BinaryValueTest) {
    std::string value("A\0B\0\0C", 6);
    ASSERT_TRUE(kv->put("key1", value) == status::OK) << pmemobj_errormsg();
    std::string value_out;
    ASSERT_TRUE(kv->get("key1", &value_out) == status::OK && (value_out.length() == 6) && (value_out == value));
}

TEST_F(VSMapTest, EmptyKeyTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(kv->put("", "empty") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->put(" ", "single-space") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    ASSERT_TRUE(kv->put("\t\t", "two-tab") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 3);
    std::string value1;
    std::string value2;
    std::string value3;
    ASSERT_TRUE(kv->exists(""));
    ASSERT_TRUE(kv->get("", &value1) == status::OK && value1 == "empty");
    ASSERT_TRUE(kv->exists(" "));
    ASSERT_TRUE(kv->get(" ", &value2) == status::OK && value2 == "single-space");
    ASSERT_TRUE(kv->exists("\t\t"));
    ASSERT_TRUE(kv->get("\t\t", &value3) == status::OK && value3 == "two-tab");
}

TEST_F(VSMapTest, EmptyValueTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(kv->put("empty", "") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->put("single-space", " ") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    ASSERT_TRUE(kv->put("two-tab", "\t\t") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 3);
    std::string value1;
    std::string value2;
    std::string value3;
    ASSERT_TRUE(kv->get("empty", &value1) == status::OK && value1 == "");
    ASSERT_TRUE(kv->get("single-space", &value2) == status::OK && value2 == " ");
    ASSERT_TRUE(kv->get("two-tab", &value3) == status::OK && value3 == "\t\t");
}

TEST_F(VSMapTest, GetAppendToExternalValueTest) {
    ASSERT_TRUE(kv->put("key1", "cool") == status::OK) << pmemobj_errormsg();
    std::string value = "super";
    ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "supercool");
}

TEST_F(VSMapTest, GetHeadlessTest) {
    ASSERT_TRUE(!kv->exists("waldo"));
    std::string value;
    ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, GetMultipleTest) {
    ASSERT_TRUE(kv->put("abc", "A1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("def", "B2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("hij", "C3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("jkl", "D4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("mno", "E5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 5);
    ASSERT_TRUE(kv->exists("abc"));
    std::string value1;
    ASSERT_TRUE(kv->get("abc", &value1) == status::OK && value1 == "A1");
    ASSERT_TRUE(kv->exists("def"));
    std::string value2;
    ASSERT_TRUE(kv->get("def", &value2) == status::OK && value2 == "B2");
    ASSERT_TRUE(kv->exists("hij"));
    std::string value3;
    ASSERT_TRUE(kv->get("hij", &value3) == status::OK && value3 == "C3");
    ASSERT_TRUE(kv->exists("jkl"));
    std::string value4;
    ASSERT_TRUE(kv->get("jkl", &value4) == status::OK && value4 == "D4");
    ASSERT_TRUE(kv->exists("mno"));
    std::string value5;
    ASSERT_TRUE(kv->get("mno", &value5) == status::OK && value5 == "E5");
}

TEST_F(VSMapTest, GetMultiple2Test) {
    ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("key2", "value2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("key3", "value3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->remove("key2") == status::OK);
    ASSERT_TRUE(kv->put("key3", "VALUE3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    std::string value1;
    ASSERT_TRUE(kv->get("key1", &value1) == status::OK && value1 == "value1");
    std::string value2;
    ASSERT_TRUE(kv->get("key2", &value2) == status::NOT_FOUND);
    std::string value3;
    ASSERT_TRUE(kv->get("key3", &value3) == status::OK && value3 == "VALUE3");
}

TEST_F(VSMapTest, GetNonexistentTest) {
    ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(!kv->exists("waldo"));
    std::string value;
    ASSERT_TRUE(kv->get("waldo", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, PutTest) {
    ASSERT_TRUE(kv->count() == 0);

    std::string value;
    ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->get("key1", &value) == status::OK && value == "value1");

    std::string new_value;
    ASSERT_TRUE(kv->put("key1", "VALUE1") == status::OK) << pmemobj_errormsg();           // same size
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->get("key1", &new_value) == status::OK && new_value == "VALUE1");

    std::string new_value2;
    ASSERT_TRUE(kv->put("key1", "new_value") == status::OK) << pmemobj_errormsg();        // longer size
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->get("key1", &new_value2) == status::OK && new_value2 == "new_value");

    std::string new_value3;
    ASSERT_TRUE(kv->put("key1", "?") == status::OK) << pmemobj_errormsg();                // shorter size
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->get("key1", &new_value3) == status::OK && new_value3 == "?");
}

TEST_F(VSMapTest, PutKeysOfDifferentSizesTest) {
    std::string value;
    ASSERT_TRUE(kv->put("123456789ABCDE", "A") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->get("123456789ABCDE", &value) == status::OK && value == "A");

    std::string value2;
    ASSERT_TRUE(kv->put("123456789ABCDEF", "B") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    ASSERT_TRUE(kv->get("123456789ABCDEF", &value2) == status::OK && value2 == "B");

    std::string value3;
    ASSERT_TRUE(kv->put("12345678ABCDEFG", "C") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 3);
    ASSERT_TRUE(kv->get("12345678ABCDEFG", &value3) == status::OK && value3 == "C");

    std::string value4;
    ASSERT_TRUE(kv->put("123456789", "D") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 4);
    ASSERT_TRUE(kv->get("123456789", &value4) == status::OK && value4 == "D");

    std::string value5;
    ASSERT_TRUE(kv->put("123456789ABCDEFGHI", "E") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 5);
    ASSERT_TRUE(kv->get("123456789ABCDEFGHI", &value5) == status::OK && value5 == "E");
}

TEST_F(VSMapTest, PutValuesOfDifferentSizesTest) {
    std::string value;
    ASSERT_TRUE(kv->put("A", "123456789ABCDE") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->get("A", &value) == status::OK && value == "123456789ABCDE");

    std::string value2;
    ASSERT_TRUE(kv->put("B", "123456789ABCDEF") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    ASSERT_TRUE(kv->get("B", &value2) == status::OK && value2 == "123456789ABCDEF");

    std::string value3;
    ASSERT_TRUE(kv->put("C", "12345678ABCDEFG") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 3);
    ASSERT_TRUE(kv->get("C", &value3) == status::OK && value3 == "12345678ABCDEFG");

    std::string value4;
    ASSERT_TRUE(kv->put("D", "123456789") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 4);
    ASSERT_TRUE(kv->get("D", &value4) == status::OK && value4 == "123456789");

    std::string value5;
    ASSERT_TRUE(kv->put("E", "123456789ABCDEFGHI") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 5);
    ASSERT_TRUE(kv->get("E", &value5) == status::OK && value5 == "123456789ABCDEFGHI");
}

TEST_F(VSMapTest, RemoveAllTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(!kv->exists("tmpkey"));
    std::string value;
    ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, RemoveAndInsertTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(kv->put("tmpkey", "tmpvalue1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->remove("tmpkey") == status::OK);
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(!kv->exists("tmpkey"));
    std::string value;
    ASSERT_TRUE(kv->get("tmpkey", &value) == status::NOT_FOUND);
    ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->exists("tmpkey1"));
    ASSERT_TRUE(kv->get("tmpkey1", &value) == status::OK && value == "tmpvalue1");
    ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(!kv->exists("tmpkey1"));
    ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
}

TEST_F(VSMapTest, RemoveExistingTest) {
    ASSERT_TRUE(kv->count() == 0);
    ASSERT_TRUE(kv->put("tmpkey1", "tmpvalue1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->put("tmpkey2", "tmpvalue2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 2);
    ASSERT_TRUE(kv->remove("tmpkey1") == status::OK);
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(kv->remove("tmpkey1") == status::NOT_FOUND);
    ASSERT_TRUE(kv->count() == 1);
    ASSERT_TRUE(!kv->exists("tmpkey1"));
    std::string value;
    ASSERT_TRUE(kv->get("tmpkey1", &value) == status::NOT_FOUND);
    ASSERT_TRUE(kv->exists("tmpkey2"));
    ASSERT_TRUE(kv->get("tmpkey2", &value) == status::OK && value == "tmpvalue2");
}

TEST_F(VSMapTest, RemoveHeadlessTest) {
    ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
}

TEST_F(VSMapTest, RemoveNonexistentTest) {
    ASSERT_TRUE(kv->put("key1", "value1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->remove("nada") == status::NOT_FOUND);
    ASSERT_TRUE(kv->exists("key1"));
}

TEST_F(VSMapTest, UsesAllTest) {
    ASSERT_TRUE(kv->put("1", "one") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("2", "two") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->all([&](const std::string& k) { x.append("<").append(k).append(">,"); });
    ASSERT_TRUE(x == "<1>,<2>,<记!>,");

    x = "";
    kv->all([&](const char *k, size_t kb) { x.append("<").append(std::string(k, kb)).append(">,"); });
    ASSERT_TRUE(x == "<1>,<2>,<记!>,");

    x = "";
    kv->all(&x, [](void *context, const char *k, size_t kb) {
        const auto c = ((std::string*) context);
        c->append("<").append(std::string(k, kb)).append(">,");
    });
    ASSERT_TRUE(x == "<1>,<2>,<记!>,");
}

TEST_F(VSMapTest, UsesAllAboveTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->all_above("B", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "BB,BC,");

    x = "";
    kv->all_above("", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,");

    x = "";
    kv->all_above("ZZZ", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->all_above("B", [&](const char *k, size_t kb) { x.append(std::string(k, kb)).append(","); });
    ASSERT_TRUE(x == "BB,BC,");

    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();
    x = "";
    kv->all_above(&x, "B", [](void *context, const char *k, size_t kb) {
        const auto c = ((std::string*) context);
        c->append(std::string(k, kb)).append(",");
    });
    ASSERT_TRUE(x == "BB,BC,记!,");
}

TEST_F(VSMapTest, UsesAllBelowTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->all_below("B", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,");

    x = "";
    kv->all_below("", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->all_below("ZZZZ", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,");

    x = "";
    kv->all_below("B", [&](const char *k, size_t kb) { x.append(std::string(k, kb)).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,");

    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();
    x = "";
    kv->all_below(&x, "\xFF", [](void *context, const char *k, size_t kb) {
        const auto c = ((std::string*) context);
        c->append(std::string(k, kb)).append(",");
    });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,记!,");
}

TEST_F(VSMapTest, UsesAllBetweenTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->all_between("A", "B", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "AB,AC,");

    x = "";
    kv->all_between("", "ZZZ", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,");

    x = "";
    kv->all_between("", "A", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->all_between("", "B", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,");

    x = "";
    kv->all_between("B", "ZZZ", [&](const std::string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "BB,BC,");

    x = "";
    kv->all_between("", "", [&](const std::string& k) { x.append("<").append(k).append(">,"); });
    kv->all_between("A", "A", [&](const std::string& k) { x.append("<").append(k).append(">,"); });
    kv->all_between("AC", "A", [&](const std::string& k) { x.append("<").append(k).append(">,"); });
    kv->all_between("B", "A", [&](const std::string& k) { x.append("<").append(k).append(">,"); });
    kv->all_between("BD", "A", [&](const std::string& k) { x.append("<").append(k).append(">,"); });
    kv->all_between("ZZZ", "B", [&](const std::string& k) { x.append("<").append(k).append(">,"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->all_between("A", "B", [&](const char *k, size_t kb) { x.append(std::string(k, kb)).append(","); });
    ASSERT_TRUE(x == "AB,AC,");

    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();
    x = "";
    kv->all_between(&x, "B", "\xFF", [](void *context, const char *k, size_t kb) {
        const auto c = ((std::string*) context);
        c->append(std::string(k, kb)).append(",");
    });
    ASSERT_TRUE(x == "BB,BC,记!,");
}

TEST_F(VSMapTest, UsesCountTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BD", "7") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->count() == 7);

    ASSERT_TRUE(kv->count_above("") == 7);
    ASSERT_TRUE(kv->count_above("A") == 6);
    ASSERT_TRUE(kv->count_above("B") == 3);
    ASSERT_TRUE(kv->count_above("BC") == 1);
    ASSERT_TRUE(kv->count_above("BD") == 0);
    ASSERT_TRUE(kv->count_above("Z") == 0);

    ASSERT_TRUE(kv->count_below("") == 0);
    ASSERT_TRUE(kv->count_below("A") == 0);
    ASSERT_TRUE(kv->count_below("B") == 3);
    ASSERT_TRUE(kv->count_below("BD") == 6);
    ASSERT_TRUE(kv->count_below("ZZZZZ") == 7);

    ASSERT_TRUE(kv->count_between("", "ZZZZ") == 7);
    ASSERT_TRUE(kv->count_between("", "A") == 0);
    ASSERT_TRUE(kv->count_between("", "B") == 3);
    ASSERT_TRUE(kv->count_between("A", "B") == 2);
    ASSERT_TRUE(kv->count_between("B", "ZZZZ") == 3);

    ASSERT_TRUE(kv->count_between("", "") == 0);
    ASSERT_TRUE(kv->count_between("A", "A") == 0);
    ASSERT_TRUE(kv->count_between("AC", "A") == 0);
    ASSERT_TRUE(kv->count_between("B", "A") == 0);
    ASSERT_TRUE(kv->count_between("BD", "A") == 0);
    ASSERT_TRUE(kv->count_between("ZZZ", "B") == 0);
}

TEST_F(VSMapTest, UsesEachTest) {
    ASSERT_TRUE(kv->put("1", "one") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("2", "two") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->each([&](const std::string& k, const std::string& v) {
        x.append("<").append(k).append(">,<").append(v).append(">|");
    });
    ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

    x = "";
    kv->each([&](const char *k, size_t kb, const char *v, size_t vb) {
        x.append("<").append(std::string(k, kb)).append(">,<").append(std::string(v, vb)).append(">|");
    });
    ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

    x = "";
    kv->each(&x, [](void *context, const char *k, size_t kb, const char *v, size_t vb) {
        const auto c = ((std::string*) context);
        c->append("<").append(std::string(k, kb)).append(">,<").append(std::string(v, vb)).append(">|");
    });
    ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");
}

TEST_F(VSMapTest, UsesEachAboveTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->each_above("B", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "BB,5|BC,6|");

    x = "";
    kv->each_above("", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

    x = "";
    kv->each_above("ZZZ", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->each_above("B", [&](const char *k, size_t kb, const char *v, size_t vb) {
        x.append(std::string(k, kb)).append(",").append(std::string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "BB,5|BC,6|");

    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();
    x = "";
    kv->each_above(&x, "B", [](void *context, const char *k, size_t kb, const char *v, size_t vb) {
        const auto c = ((std::string*) context);
        c->append(std::string(k, kb)).append(",").append(std::string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "BB,5|BC,6|记!,RR|");
}

TEST_F(VSMapTest, UsesEachBelowTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->each_below("AC", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|");

    x = "";
    kv->each_below("", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->each_below("ZZZZ", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

    x = "";
    kv->each_below("AC", [&](const char *k, size_t kb, const char *v, size_t vb) {
        x.append(std::string(k, kb)).append(",").append(std::string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "A,1|AB,2|");

    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();
    x = "";
    kv->each_below(&x, "\xFF", [](void *context, const char *k, size_t kb, const char *v, size_t vb) {
        const auto c = ((std::string*) context);
        c->append(std::string(k, kb)).append(",").append(std::string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

TEST_F(VSMapTest, UsesEachBetweenTest) {
    ASSERT_TRUE(kv->put("A", "1") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AB", "2") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("AC", "3") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("B", "4") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BB", "5") == status::OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->put("BC", "6") == status::OK) << pmemobj_errormsg();

    std::string x;
    kv->each_between("A", "B", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "AB,2|AC,3|");

    x = "";
    kv->each_between("", "ZZZ", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

    x = "";
    kv->each_between("", "A", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->each_between("", "B", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|");

    x = "";
    kv->each_between("", "", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->each_between("A", "A", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->each_between("AC", "A", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->each_between("B", "A", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->each_between("BD", "A", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->each_between("ZZZ", "A", [&](const std::string& k, const std::string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->each_between("A", "B", [&](const char *k, size_t kb, const char *v, size_t vb) {
        x.append(std::string(k, kb)).append(",").append(std::string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "AB,2|AC,3|");

    ASSERT_TRUE(kv->put("记!", "RR") == status::OK) << pmemobj_errormsg();
    x = "";
    kv->each_between(&x, "B", "\xFF", [](void *context, const char *k, size_t kb, const char *v, size_t vb) {
        const auto c = ((std::string*) context);
        c->append(std::string(k, kb)).append(",").append(std::string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "BB,5|BC,6|记!,RR|");
}

// =============================================================================================
// TEST LARGE COLLECTIONS
// =============================================================================================

const int LARGE_LIMIT = 4000000;

TEST_F(VSMapLargeTest, LargeAscendingTest) {
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->put(istr, (istr + "!")) == status::OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == (istr + "!"));
    }
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == (istr + "!"));
    }
    ASSERT_TRUE(kv->count() == LARGE_LIMIT);
}

TEST_F(VSMapLargeTest, LargeDescendingTest) {
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->put(istr, ("ABC" + istr)) == status::OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == ("ABC" + istr));
    }
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->get(istr, &value) == status::OK && value == ("ABC" + istr));
    }
    ASSERT_TRUE(kv->count() == LARGE_LIMIT);
}
