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
#include "../../src/engines-experimental/stree.h"
#include <libpmemobj.h>

using namespace pmemkv;
using namespace pmemkv::stree;

const std::string PATH = "/dev/shm/pmemkv";
const size_t SIZE = 1024ull * 1024ull * 512ull;
const size_t LARGE_SIZE = 1024ull * 1024ull * 1024ull * 2ull;

template<size_t POOL_SIZE>
class STreeBaseTest : public testing::Test {
  public:
    KVEngine* kv;

    STreeBaseTest() {
        std::remove(PATH.c_str());
        Start();
    }

    ~STreeBaseTest() {
        delete kv;
    }
    void Restart() {
        delete kv;
        Start();
    }
  protected:
    void Start() {
        char config[255];
        auto n = sprintf(config, "{\"path\": \"%s\", \"size\" : %lu}", PATH.c_str(), POOL_SIZE);

        if (n < 0)
            throw std::runtime_error("sprintf failed");

        kv = new KVEngine(nullptr, "stree", config);
    }
};

typedef STreeBaseTest<SIZE> STreeTest;
typedef STreeBaseTest<LARGE_SIZE> STreeLargeTest;

TEST_F(STreeTest, SimpleTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("key1"));
    std::string value;
    ASSERT_TRUE(kv->Get("key1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("key1"));
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");
}

TEST_F(STreeTest, BinaryKeyTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("a"));
    ASSERT_TRUE(kv->Put("a", "should_not_change") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("a"));
    std::string key1 = std::string("a\0b", 3);
    ASSERT_TRUE(!kv->Exists(key1));
    ASSERT_TRUE(kv->Put(key1, "stuff") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Exists("a"));
    ASSERT_TRUE(kv->Exists(key1));
    std::string value;
    ASSERT_TRUE(kv->Get(key1, &value) == OK);
    ASSERT_EQ(value, "stuff");
    std::string value2;
    ASSERT_TRUE(kv->Get("a", &value2) == OK);
    ASSERT_EQ(value2, "should_not_change");
    ASSERT_TRUE(kv->Remove(key1) == OK);
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Exists("a"));
    ASSERT_TRUE(!kv->Exists(key1));
    std::string value3;
    ASSERT_TRUE(kv->Get(key1, &value3) == NOT_FOUND);
    ASSERT_TRUE(kv->Get("a", &value3) == OK && value3 == "should_not_change");
}

TEST_F(STreeTest, BinaryValueTest) {
    std::string value("A\0B\0\0C", 6);
    ASSERT_TRUE(kv->Put("key1", value) == OK) << pmemobj_errormsg();
    std::string value_out;
    ASSERT_TRUE(kv->Get("key1", &value_out) == OK && (value_out.length() == 6) && (value_out == value));
}

TEST_F(STreeTest, EmptyKeyTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("", "empty") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put(" ", "single-space") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Put("\t\t", "two-tab") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    std::string value1;
    std::string value2;
    std::string value3;
    ASSERT_TRUE(kv->Exists(""));
    ASSERT_TRUE(kv->Get("", &value1) == OK && value1 == "empty");
    ASSERT_TRUE(kv->Exists(" "));
    ASSERT_TRUE(kv->Get(" ", &value2) == OK && value2 == "single-space");
    ASSERT_TRUE(kv->Exists("\t\t"));
    ASSERT_TRUE(kv->Get("\t\t", &value3) == OK && value3 == "two-tab");
}

TEST_F(STreeTest, EmptyValueTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("empty", "") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put("single-space", " ") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Put("two-tab", "\t\t") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    std::string value1;
    std::string value2;
    std::string value3;
    ASSERT_TRUE(kv->Get("empty", &value1) == OK && value1 == "");
    ASSERT_TRUE(kv->Get("single-space", &value2) == OK && value2 == " ");
    ASSERT_TRUE(kv->Get("two-tab", &value3) == OK && value3 == "\t\t");
}

TEST_F(STreeTest, GetAppendToExternalValueTest) {
    ASSERT_TRUE(kv->Put("key1", "cool") == OK) << pmemobj_errormsg();
    std::string value = "super";
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "supercool");
}

TEST_F(STreeTest, GetHeadlessTest) {
    ASSERT_TRUE(!kv->Exists("waldo"));
    std::string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(STreeTest, GetMultipleTest) {
    ASSERT_TRUE(kv->Put("abc", "A1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("def", "B2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("hij", "C3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("jkl", "D4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("mno", "E5") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 5);
    ASSERT_TRUE(kv->Exists("abc"));
    std::string value1;
    ASSERT_TRUE(kv->Get("abc", &value1) == OK && value1 == "A1");
    ASSERT_TRUE(kv->Exists("def"));
    std::string value2;
    ASSERT_TRUE(kv->Get("def", &value2) == OK && value2 == "B2");
    ASSERT_TRUE(kv->Exists("hij"));
    std::string value3;
    ASSERT_TRUE(kv->Get("hij", &value3) == OK && value3 == "C3");
    ASSERT_TRUE(kv->Exists("jkl"));
    std::string value4;
    ASSERT_TRUE(kv->Get("jkl", &value4) == OK && value4 == "D4");
    ASSERT_TRUE(kv->Exists("mno"));
    std::string value5;
    ASSERT_TRUE(kv->Get("mno", &value5) == OK && value5 == "E5");
}

TEST_F(STreeTest, GetMultiple2Test) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key2", "value2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key3", "value3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("key2") == OK);
    ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    std::string value1;
    ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
    std::string value2;
    ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
    std::string value3;
    ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");
}

TEST_F(STreeTest, GetNonexistentTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(!kv->Exists("waldo"));
    std::string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(STreeTest, PutTest) {
    ASSERT_TRUE(kv->Count() == 0);

    std::string value;
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");

    std::string new_value;
    ASSERT_TRUE(kv->Put("key1", "VALUE1") == OK) << pmemobj_errormsg();           // same size
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &new_value) == OK && new_value == "VALUE1");

    std::string new_value2;
    ASSERT_TRUE(kv->Put("key1", "new_value") == OK) << pmemobj_errormsg();        // longer size
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &new_value2) == OK && new_value2 == "new_value");

    std::string new_value3;
    ASSERT_TRUE(kv->Put("key1", "?") == OK) << pmemobj_errormsg();                // shorter size
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("key1", &new_value3) == OK && new_value3 == "?");
}

TEST_F(STreeTest, PutKeysOfDifferentSizesTest) {
    std::string value;
    ASSERT_TRUE(kv->Put("123456789ABCDE", "A") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("123456789ABCDE", &value) == OK && value == "A");

    std::string value2;
    ASSERT_TRUE(kv->Put("123456789ABCDEF", "B") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Get("123456789ABCDEF", &value2) == OK && value2 == "B");

    std::string value3;
    ASSERT_TRUE(kv->Put("12345678ABCDEFG", "C") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    ASSERT_TRUE(kv->Get("12345678ABCDEFG", &value3) == OK && value3 == "C");

    std::string value4;
    ASSERT_TRUE(kv->Put("123456789", "D") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 4);
    ASSERT_TRUE(kv->Get("123456789", &value4) == OK && value4 == "D");

    std::string value5;
    ASSERT_TRUE(kv->Put("123456789ABCDEFGHI", "E") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 5);
    ASSERT_TRUE(kv->Get("123456789ABCDEFGHI", &value5) == OK && value5 == "E");
}

TEST_F(STreeTest, PutValuesOfDifferentSizesTest) {
    std::string value;
    ASSERT_TRUE(kv->Put("A", "123456789ABCDE") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Get("A", &value) == OK && value == "123456789ABCDE");

    std::string value2;
    ASSERT_TRUE(kv->Put("B", "123456789ABCDEF") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);
    ASSERT_TRUE(kv->Get("B", &value2) == OK && value2 == "123456789ABCDEF");

    std::string value3;
    ASSERT_TRUE(kv->Put("C", "12345678ABCDEFG") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 3);
    ASSERT_TRUE(kv->Get("C", &value3) == OK && value3 == "12345678ABCDEFG");

    std::string value4;
    ASSERT_TRUE(kv->Put("D", "123456789") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 4);
    ASSERT_TRUE(kv->Get("D", &value4) == OK && value4 == "123456789");

    std::string value5;
    ASSERT_TRUE(kv->Put("E", "123456789ABCDEFGHI") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 5);
    ASSERT_TRUE(kv->Get("E", &value5) == OK && value5 == "123456789ABCDEFGHI");
}

TEST_F(STreeTest, RemoveAllTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("tmpkey"));
    std::string value;
    ASSERT_TRUE(kv->Get("tmpkey", &value) == NOT_FOUND);
}

TEST_F(STreeTest, RemoveAndInsertTest) {
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    ASSERT_TRUE(kv->Count() == 0);
    ASSERT_TRUE(!kv->Exists("tmpkey"));
    std::string value;
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

TEST_F(STreeTest, RemoveExistingTest) {
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
    std::string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Exists("tmpkey2"));
    ASSERT_TRUE(kv->Get("tmpkey2", &value) == OK && value == "tmpvalue2");
}

TEST_F(STreeTest, RemoveHeadlessTest) {
    ASSERT_TRUE(kv->Remove("nada") == NOT_FOUND);
}

TEST_F(STreeTest, RemoveNonexistentTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("nada") == NOT_FOUND);
    ASSERT_TRUE(kv->Exists("key1"));
}

TEST_F(STreeTest, UsesAllTest) {
    ASSERT_TRUE(kv->Put("2", "1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put("记!", "RR") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);

    std::string result;
    kv->All(&result, [](void* context, int kb, const char* k) {
        const auto c = ((std::string*) context);
        c->append("<");
        c->append(std::string(k, kb));
        c->append(">,");
    });
    ASSERT_TRUE(result == "<记!>,<2>,");
}

TEST_F(STreeTest, UsesEachTest) {
    ASSERT_TRUE(kv->Put("1", "2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 1);
    ASSERT_TRUE(kv->Put("RR", "记!") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Count() == 2);

    std::string result;
    kv->Each(&result, [](void* context, int kb, const char* k, int vb, const char* v) {
        const auto c = ((std::string*) context);
        c->append("<");
        c->append(std::string(k, kb));
        c->append(">,<");
        c->append(std::string(v, vb));
        c->append(">|");
    });
    ASSERT_TRUE(result == "<1>,<2>|<RR>,<记!>|");
}

// =============================================================================================
// TEST RECOVERY OF SINGLE-LEAF TREE
// =============================================================================================

TEST_F(STreeTest, GetHeadlessAfterRecoveryTest) {
    Restart();
    std::string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(STreeTest, GetMultipleAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("abc", "A1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("def", "B2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("hij", "C3") == OK) << pmemobj_errormsg();
    Restart();
    ASSERT_TRUE(kv->Put("jkl", "D4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("mno", "E5") == OK) << pmemobj_errormsg();
    std::string value1;
    ASSERT_TRUE(kv->Get("abc", &value1) == OK && value1 == "A1");
    std::string value2;
    ASSERT_TRUE(kv->Get("def", &value2) == OK && value2 == "B2");
    std::string value3;
    ASSERT_TRUE(kv->Get("hij", &value3) == OK && value3 == "C3");
    std::string value4;
    ASSERT_TRUE(kv->Get("jkl", &value4) == OK && value4 == "D4");
    std::string value5;
    ASSERT_TRUE(kv->Get("mno", &value5) == OK && value5 == "E5");
}

TEST_F(STreeTest, GetMultiple2AfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key2", "value2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key3", "value3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("key2") == OK);
    ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK) << pmemobj_errormsg();
    Restart();
    std::string value1;
    ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
    std::string value2;
    ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
    std::string value3;
    ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");
}

TEST_F(STreeTest, GetNonexistentAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    Restart();
    std::string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(STreeTest, PutAfterRecoveryTest) {
    std::string value;
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");

    std::string new_value;
    ASSERT_TRUE(kv->Put("key1", "VALUE1") == OK) << pmemobj_errormsg();           // same size
    ASSERT_TRUE(kv->Get("key1", &new_value) == OK && new_value == "VALUE1");
    Restart();

    std::string new_value2;
    ASSERT_TRUE(kv->Put("key1", "new_value") == OK) << pmemobj_errormsg();        // longer size
    ASSERT_TRUE(kv->Get("key1", &new_value2) == OK && new_value2 == "new_value");

    std::string new_value3;
    ASSERT_TRUE(kv->Put("key1", "?") == OK) << pmemobj_errormsg();                // shorter size
    ASSERT_TRUE(kv->Get("key1", &new_value3) == OK && new_value3 == "?");
}

TEST_F(STreeTest, RemoveAllAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    Restart();
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    std::string value;
    ASSERT_TRUE(kv->Get("tmpkey", &value) == NOT_FOUND);
}

TEST_F(STreeTest, RemoveAndInsertAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    Restart();
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    std::string value;
    ASSERT_TRUE(kv->Get("tmpkey", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == OK && value == "tmpvalue1");
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK);
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
}

TEST_F(STreeTest, RemoveExistingAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("tmpkey2", "tmpvalue2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK);
    Restart();
    ASSERT_TRUE(kv->Remove("tmpkey1") == NOT_FOUND);
    std::string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Get("tmpkey2", &value) == OK && value == "tmpvalue2");
}

TEST_F(STreeTest, RemoveHeadlessAfterRecoveryTest) {
    Restart();
    ASSERT_TRUE(kv->Remove("nada") == NOT_FOUND);
}

TEST_F(STreeTest, RemoveNonexistentAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    Restart();
    ASSERT_TRUE(kv->Remove("nada") == NOT_FOUND);
}

// =============================================================================================
// TEST TREE WITH SINGLE INNER NODE
// =============================================================================================

const size_t INNER_ENTRIES = DEGREE - 1;
const size_t LEAF_ENTRIES = DEGREE - 1;
const size_t SINGLE_INNER_LIMIT = LEAF_ENTRIES * (INNER_ENTRIES - 1);

TEST_F(STreeTest, SingleInnerNodeAscendingTest) {
    for (int i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeAscendingTest2) {
    for (int i = 0; i < SINGLE_INNER_LIMIT; i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = 0; i < SINGLE_INNER_LIMIT; i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingTest) {
    for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingTest2) {
    for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

// =============================================================================================
// TEST RECOVERY OF TREE WITH SINGLE INNER NODE
// =============================================================================================

TEST_F(STreeTest, SingleInnerNodeAscendingAfterRecoveryTest) {
    for (int i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Restart();
    for (int i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeAscendingAfterRecoveryTest2) {
    for (int i = 0; i < SINGLE_INNER_LIMIT; i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Restart();
    for (int i = 0; i < SINGLE_INNER_LIMIT; i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingAfterRecoveryTest) {
    for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Restart();
    for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

TEST_F(STreeTest, SingleInnerNodeDescendingAfterRecoveryTest2) {
    for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Restart();
    for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    ASSERT_TRUE(kv->Count() == SINGLE_INNER_LIMIT);
}

// =============================================================================================
// TEST LARGE TREE
// =============================================================================================

const int LARGE_LIMIT = 4000000;

TEST_F(STreeLargeTest, LargeAscendingTest) {
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, (istr + "!")) == OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == (istr + "!"));
    }
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == (istr + "!"));
    }
    ASSERT_TRUE(kv->Count() == LARGE_LIMIT);
}

TEST_F(STreeLargeTest, LargeDescendingTest) {
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, ("ABC" + istr)) == OK) << pmemobj_errormsg();
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
    }
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
    }
    ASSERT_TRUE(kv->Count() == LARGE_LIMIT);
}

// =============================================================================================
// TEST RECOVERY OF LARGE TREE
// =============================================================================================

TEST_F(STreeLargeTest, LargeAscendingAfterRecoveryTest) {
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, (istr + "!")) == OK) << pmemobj_errormsg();
    }
    Restart();
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == (istr + "!"));
    }
    ASSERT_TRUE(kv->Count() == LARGE_LIMIT);
}

TEST_F(STreeLargeTest, LargeDescendingAfterRecoveryTest) {
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        std::string istr = std::to_string(i);
        ASSERT_TRUE(kv->Put(istr, ("ABC" + istr)) == OK) << pmemobj_errormsg();
    }
    Restart();
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        std::string istr = std::to_string(i);
        std::string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
    }
    ASSERT_TRUE(kv->Count() == LARGE_LIMIT);
}
