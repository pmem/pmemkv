/*
 * Copyright 2017-2018, Intel Corporation
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
#include "../../src/engines/vmap.h"

using namespace pmemkv::vmap;

const string PATH = "/dev/shm";
const size_t SIZE = 1024ull * 1024ull * 512ull;
const size_t LARGE_SIZE = 1024ull * 1024ull * 1024ull * 2ull;

template<size_t POOL_SIZE>
class VMapBaseTest : public testing::Test {
public:
    VMap* kv;

    VMapBaseTest() {
        kv = new VMap(PATH, POOL_SIZE);
    }

    ~VMapBaseTest() {
        delete kv;
    }
};

using VMapTest = VMapBaseTest<SIZE>;
using VMapLargeTest = VMapBaseTest<LARGE_SIZE>;

// =============================================================================================
// TEST SMALL COLLECTIONS
// =============================================================================================

TEST_F(VMapTest, SimpleTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("key1"));
    string value;
    ASSERT_TRUE(kv->Get("key1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("key1"));
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");
    value = "";
    kv->Get("key1", [&](int vb, const char* v) { value.append(v, vb); });
    ASSERT_TRUE(value == "value1");
    value = "";
    kv->Get("key1", [&](const string& v) { value.append(v); });
    ASSERT_TRUE(value == "value1");
}

TEST_F(VMapTest, BinaryKeyTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("a"));
    ASSERT_TRUE(kv->Put("a", "should_not_change") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("a"));
    string key1 = string("a\0b", 3);
    ASSERT_TRUE(!kv->Exists(key1));
    ASSERT_TRUE(kv->Put(key1, "stuff") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Exists("a"));
    ASSERT_TRUE(kv->Exists(key1));
    string value;
    ASSERT_TRUE(kv->Get(key1, &value) == OK);
    ASSERT_EQ(value, "stuff");
    string value2;
    ASSERT_TRUE(kv->Get("a", &value2) == OK);
    ASSERT_EQ(value2, "should_not_change");
    ASSERT_TRUE(kv->Remove(key1) == OK);
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("a"));
    ASSERT_TRUE(!kv->Exists(key1));
    string value3;
    ASSERT_TRUE(kv->Get(key1, &value3) == NOT_FOUND);
    ASSERT_TRUE(kv->Get("a", &value3) == OK && value3 == "should_not_change");
}

TEST_F(VMapTest, BinaryValueTest) {
    string value("A\0B\0\0C", 6);
    ASSERT_TRUE(kv->Put("key1", value) == OK) << pmemobj_errormsg();
    string value_out;
    ASSERT_TRUE(kv->Get("key1", &value_out) == OK && (value_out.length() == 6) && (value_out == value));
}

TEST_F(VMapTest, EmptyKeyTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("", "empty") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put(" ", "single-space") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Put("\t\t", "two-tab") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    string value1;
    string value2;
    string value3;
    ASSERT_TRUE(kv->Exists(""));
    ASSERT_TRUE(kv->Get("", &value1) == OK && value1 == "empty");
    ASSERT_TRUE(kv->Exists(" "));
    ASSERT_TRUE(kv->Get(" ", &value2) == OK && value2 == "single-space");
    ASSERT_TRUE(kv->Exists("\t\t"));
    ASSERT_TRUE(kv->Get("\t\t", &value3) == OK && value3 == "two-tab");
}

TEST_F(VMapTest, EmptyValueTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("empty", "") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put("single-space", " ") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Put("two-tab", "\t\t") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    string value1;
    string value2;
    string value3;
    ASSERT_TRUE(kv->Get("empty", &value1) == OK && value1 == "");
    ASSERT_TRUE(kv->Get("single-space", &value2) == OK && value2 == " ");
    ASSERT_TRUE(kv->Get("two-tab", &value3) == OK && value3 == "\t\t");
}

TEST_F(VMapTest, GetAppendToExternalValueTest) {
    ASSERT_TRUE(kv->Put("key1", "cool") == OK) << pmemobj_errormsg();
    string value = "super";
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "supercool");
}

TEST_F(VMapTest, GetHeadlessTest) {
    ASSERT_TRUE(!kv->Exists("waldo"));
    string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(VMapTest, GetMultipleTest) {
    ASSERT_TRUE(kv->Put("abc", "A1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("def", "B2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("hij", "C3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("jkl", "D4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("mno", "E5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 5);
    ASSERT_TRUE(kv->Exists("abc"));
    string value1;
    ASSERT_TRUE(kv->Get("abc", &value1) == OK && value1 == "A1");
    ASSERT_TRUE(kv->Exists("def"));
    string value2;
    ASSERT_TRUE(kv->Get("def", &value2) == OK && value2 == "B2");
    ASSERT_TRUE(kv->Exists("hij"));
    string value3;
    ASSERT_TRUE(kv->Get("hij", &value3) == OK && value3 == "C3");
    ASSERT_TRUE(kv->Exists("jkl"));
    string value4;
    ASSERT_TRUE(kv->Get("jkl", &value4) == OK && value4 == "D4");
    ASSERT_TRUE(kv->Exists("mno"));
    string value5;
    ASSERT_TRUE(kv->Get("mno", &value5) == OK && value5 == "E5");
}

TEST_F(VMapTest, GetMultiple2Test) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key2", "value2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key3", "value3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("key2") == OK);
    ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    string value1;
    ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
    string value2;
    ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
    string value3;
    ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");
}

TEST_F(VMapTest, GetNonexistentTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(!kv->Exists("waldo"));
    string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(VMapTest, PutTest) {
    ASSERT_TRUE(kv->Count() == 0);

    string value;
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");

    string new_value;
    ASSERT_TRUE(kv->Put("key1", "VALUE1") == OK) << pmemobj_errormsg();           // same size
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &new_value) == OK && new_value == "VALUE1");

    string new_value2;
    ASSERT_TRUE(kv->Put("key1", "new_value") == OK) << pmemobj_errormsg();        // longer size
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &new_value2) == OK && new_value2 == "new_value");

    string new_value3;
    ASSERT_TRUE(kv->Put("key1", "?") == OK) << pmemobj_errormsg();                // shorter size
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &new_value3) == OK && new_value3 == "?");
}

TEST_F(VMapTest, PutKeysOfDifferentSizesTest) {
    string value;
    ASSERT_TRUE(kv->Put("123456789ABCDE", "A") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("123456789ABCDE", &value) == OK && value == "A");

    string value2;
    ASSERT_TRUE(kv->Put("123456789ABCDEF", "B") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Get("123456789ABCDEF", &value2) == OK && value2 == "B");

    string value3;
    ASSERT_TRUE(kv->Put("12345678ABCDEFG", "C") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    ASSERT_TRUE(kv->Get("12345678ABCDEFG", &value3) == OK && value3 == "C");

    string value4;
    ASSERT_TRUE(kv->Put("123456789", "D") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 4);
    ASSERT_TRUE(kv->Get("123456789", &value4) == OK && value4 == "D");

    string value5;
    ASSERT_TRUE(kv->Put("123456789ABCDEFGHI", "E") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 5);
    ASSERT_TRUE(kv->Get("123456789ABCDEFGHI", &value5) == OK && value5 == "E");
}

TEST_F(VMapTest, PutValuesOfDifferentSizesTest) {
    string value;
    ASSERT_TRUE(kv->Put("A", "123456789ABCDE") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("A", &value) == OK && value == "123456789ABCDE");

    string value2;
    ASSERT_TRUE(kv->Put("B", "123456789ABCDEF") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Get("B", &value2) == OK && value2 == "123456789ABCDEF");

    string value3;
    ASSERT_TRUE(kv->Put("C", "12345678ABCDEFG") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    ASSERT_TRUE(kv->Get("C", &value3) == OK && value3 == "12345678ABCDEFG");

    string value4;
    ASSERT_TRUE(kv->Put("D", "123456789") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 4);
    ASSERT_TRUE(kv->Get("D", &value4) == OK && value4 == "123456789");

    string value5;
    ASSERT_TRUE(kv->Put("E", "123456789ABCDEFGHI") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 5);
    ASSERT_TRUE(kv->Get("E", &value5) == OK && value5 == "123456789ABCDEFGHI");
}

TEST_F(VMapTest, RemoveAllTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("tmpkey"));
    string value;
    ASSERT_TRUE(kv->Get("tmpkey", &value) == NOT_FOUND);
}

TEST_F(VMapTest, RemoveAndInsertTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("tmpkey"));
    string value;
    ASSERT_TRUE(kv->Get("tmpkey", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("tmpkey1"));
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == OK && value == "tmpvalue1");
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK);
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("tmpkey1"));
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
}

TEST_F(VMapTest, RemoveExistingTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put("tmpkey2", "tmpvalue2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK);
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Remove("tmpkey1") == NOT_FOUND);
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(!kv->Exists("tmpkey1"));
    string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Exists("tmpkey2"));
    ASSERT_TRUE(kv->Get("tmpkey2", &value) == OK && value == "tmpvalue2");
}

TEST_F(VMapTest, RemoveHeadlessTest) {
    ASSERT_TRUE(kv->Remove("nada") == NOT_FOUND);
}

TEST_F(VMapTest, RemoveNonexistentTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("nada") == NOT_FOUND);
    ASSERT_TRUE(kv->Exists("key1"));
}

TEST_F(VMapTest, UsesAllTest) {
    ASSERT_TRUE(kv->Put("1", "one") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("2", "two") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();

    string x;
    kv->All([&](const string& k) { x.append("<").append(k).append(">,"); });
    ASSERT_TRUE(x == "<1>,<2>,<记!>,");

    x = "";
    kv->All([&](int kb, const char* k) { x.append("<").append(string(k, kb)).append(">,"); });
    ASSERT_TRUE(x == "<1>,<2>,<记!>,");

    x = "";
    kv->All(&x, [](void* context, int kb, const char* k) {
        const auto c = ((string*) context);
        c->append("<").append(string(k, kb)).append(">,");
    });
    ASSERT_TRUE(x == "<1>,<2>,<记!>,");
}

TEST_F(VMapTest, UsesAllAboveTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();

    string x;
    kv->AllAbove("B", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "BB,BC,");

    x = "";
    kv->AllAbove("", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,");

    x = "";
    kv->AllAbove("ZZZ", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->AllAbove("B", [&](int kb, const char* k) { x.append(string(k, kb)).append(","); });
    ASSERT_TRUE(x == "BB,BC,");

    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    x = "";
    kv->AllAbove(&x, "B", [](void* context, int kb, const char* k) {
        const auto c = ((string*) context);
        c->append(string(k, kb)).append(",");
    });
    ASSERT_TRUE(x == "BB,BC,记!,");
}

TEST_F(VMapTest, UsesAllBelowTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();

    string x;
    kv->AllBelow("B", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,");

    x = "";
    kv->AllBelow("", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->AllBelow("ZZZZ", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,");

    x = "";
    kv->AllBelow("B", [&](int kb, const char* k) { x.append(string(k, kb)).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,");

    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    x = "";
    kv->AllBelow(&x, "\xFF", [](void* context, int kb, const char* k) {
        const auto c = ((string*) context);
        c->append(string(k, kb)).append(",");
    });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,记!,");
}

TEST_F(VMapTest, UsesAllBetweenTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();

    string x;
    kv->AllBetween("A", "B", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "AB,AC,");

    x = "";
    kv->AllBetween("", "ZZZ", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,B,BB,BC,");

    x = "";
    kv->AllBetween("", "A", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->AllBetween("", "B", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "A,AB,AC,");

    x = "";
    kv->AllBetween("B", "ZZZ", [&](const string& k) { x.append(k).append(","); });
    ASSERT_TRUE(x == "BB,BC,");

    x = "";
    kv->AllBetween("", "", [&](const string& k) { x.append("<").append(k).append(">,"); });
    kv->AllBetween("A", "A", [&](const string& k) { x.append("<").append(k).append(">,"); });
    kv->AllBetween("AC", "A", [&](const string& k) { x.append("<").append(k).append(">,"); });
    kv->AllBetween("B", "A", [&](const string& k) { x.append("<").append(k).append(">,"); });
    kv->AllBetween("BD", "A", [&](const string& k) { x.append("<").append(k).append(">,"); });
    kv->AllBetween("ZZZ", "B", [&](const string& k) { x.append("<").append(k).append(">,"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->AllBetween("A", "B", [&](int kb, const char* k) { x.append(string(k, kb)).append(","); });
    ASSERT_TRUE(x == "AB,AC,");

    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    x = "";
    kv->AllBetween(&x, "B", "\xFF", [](void* context, int kb, const char* k) {
        const auto c = ((string*) context);
        c->append(string(k, kb)).append(",");
    });
    ASSERT_TRUE(x == "BB,BC,记!,");
}

TEST_F(VMapTest, UsesCountTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BD", "7") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 7);

    ASSERT_TRUE(kv->CountAbove("") == 7);
    ASSERT_TRUE(kv->CountAbove("A") == 6);
    ASSERT_TRUE(kv->CountAbove("B") == 3);
    ASSERT_TRUE(kv->CountAbove("BC") == 1);
    ASSERT_TRUE(kv->CountAbove("BD") == 0);
    ASSERT_TRUE(kv->CountAbove("Z") == 0);

    ASSERT_TRUE(kv->CountBelow("") == 0);
    ASSERT_TRUE(kv->CountBelow("A") == 0);
    ASSERT_TRUE(kv->CountBelow("B") == 3);
    ASSERT_TRUE(kv->CountBelow("BD") == 6);
    ASSERT_TRUE(kv->CountBelow("ZZZZZ") == 7);

    ASSERT_TRUE(kv->CountBetween("", "ZZZZ") == 7);
    ASSERT_TRUE(kv->CountBetween("", "A") == 0);
    ASSERT_TRUE(kv->CountBetween("", "B") == 3);
    ASSERT_TRUE(kv->CountBetween("A", "B") == 2);
    ASSERT_TRUE(kv->CountBetween("B", "ZZZZ") == 3);

    ASSERT_TRUE(kv->CountBetween("", "") == 0);
    ASSERT_TRUE(kv->CountBetween("A", "A") == 0);
    ASSERT_TRUE(kv->CountBetween("AC", "A") == 0);
    ASSERT_TRUE(kv->CountBetween("B", "A") == 0);
    ASSERT_TRUE(kv->CountBetween("BD", "A") == 0);
    ASSERT_TRUE(kv->CountBetween("ZZZ", "B") == 0);
}

TEST_F(VMapTest, UsesEachTest) {
    ASSERT_TRUE(kv->Put("1", "one") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("2", "two") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();

    string x;
    kv->Each([&](const string& k, const string& v) {
        x.append("<").append(k).append(">,<").append(v).append(">|");
    });
    ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

    x = "";
    kv->Each([&](int kb, const char* k, int vb, const char* v) {
        x.append("<").append(string(k, kb)).append(">,<").append(string(v, vb)).append(">|");
    });
    ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

    x = "";
    kv->Each(&x, [](void* context, int kb, const char* k, int vb, const char* v) {
        const auto c = ((string*) context);
        c->append("<").append(string(k, kb)).append(">,<").append(string(v, vb)).append(">|");
    });
    ASSERT_TRUE(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");
}

TEST_F(VMapTest, UsesEachAboveTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();

    string x;
    kv->EachAbove("B", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "BB,5|BC,6|");

    x = "";
    kv->EachAbove("", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

    x = "";
    kv->EachAbove("ZZZ", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->EachAbove("B", [&](int kb, const char* k, int vb, const char* v) {
        x.append(string(k, kb)).append(",").append(string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "BB,5|BC,6|");

    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    x = "";
    kv->EachAbove(&x, "B", [](void* context, int kb, const char* k, int vb, const char* v) {
        const auto c = ((string*) context);
        c->append(string(k, kb)).append(",").append(string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "BB,5|BC,6|记!,RR|");
}

TEST_F(VMapTest, UsesEachBelowTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();

    string x;
    kv->EachBelow("AC", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|");

    x = "";
    kv->EachBelow("", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->EachBelow("ZZZZ", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

    x = "";
    kv->EachBelow("AC", [&](int kb, const char* k, int vb, const char* v) {
        x.append(string(k, kb)).append(",").append(string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "A,1|AB,2|");

    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    x = "";
    kv->EachBelow(&x, "\xFF", [](void* context, int kb, const char* k, int vb, const char* v) {
        const auto c = ((string*) context);
        c->append(string(k, kb)).append(",").append(string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

TEST_F(VMapTest, UsesEachBetweenTest) {
    ASSERT_TRUE(kv->Put("A", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AB", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("AC", "3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("B", "4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BB", "5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("BC", "6") == OK) << pmemobj_errormsg();

    string x;
    kv->EachBetween("A", "B", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "AB,2|AC,3|");

    x = "";
    kv->EachBetween("", "ZZZ", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

    x = "";
    kv->EachBetween("", "A", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->EachBetween("", "B", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x == "A,1|AB,2|AC,3|");

    x = "";
    kv->EachBetween("", "", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->EachBetween("A", "A", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->EachBetween("AC", "A", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->EachBetween("B", "A", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->EachBetween("BD", "A", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    kv->EachBetween("ZZZ", "A", [&](const string& k, const string& v) { x.append(k).append(",").append(v).append("|"); });
    ASSERT_TRUE(x.empty());

    x = "";
    kv->EachBetween("A", "B", [&](int kb, const char* k, int vb, const char* v) {
        x.append(string(k, kb)).append(",").append(string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "AB,2|AC,3|");

    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    x = "";
    kv->EachBetween(&x, "B", "\xFF", [](void* context, int kb, const char* k, int vb, const char* v) {
        const auto c = ((string*) context);
        c->append(string(k, kb)).append(",").append(string(v, vb)).append("|");
    });
    ASSERT_TRUE(x == "BB,5|BC,6|记!,RR|");
}

// =============================================================================================
// TEST LARGE COLLECTIONS
// =============================================================================================

const int LARGE_LIMIT = 4000000;

TEST_F(VMapLargeTest, LargeAscendingTest) {
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, (istr + "!")) == OK) << pmemobj_errormsg();
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == (istr + "!"));
    }
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == (istr + "!"));
    }
    ASSERT_TRUE(kv->Count() == LARGE_LIMIT);
}

TEST_F(VMapLargeTest, LargeDescendingTest) {
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, ("ABC" + istr)) == OK) << pmemobj_errormsg();
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
    }
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
    }
    ASSERT_TRUE(kv->Count() == LARGE_LIMIT);
}
