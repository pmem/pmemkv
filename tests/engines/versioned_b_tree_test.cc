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
#include "../../src/engines/versioned_b_tree.h"

using namespace pmemkv::versioned_b_tree;

const string PATH = "/dev/shm/pmemkv";
const size_t SIZE = 1024ull * 1024ull * 512ull;
const size_t LARGE_SIZE = 1024ull * 1024ull * 1024ull * 2ull;

template <size_t POOL_SIZE>
class BTreeEngineBaseTest : public testing::Test {
public:
    BTreeEngine* kv;

    BTreeEngineBaseTest() {
        std::remove(PATH.c_str());
        Open();
    }

    ~BTreeEngineBaseTest() {
        delete kv;
    }

    void Reopen() {
        delete kv;
        Open();
    }

protected:
    void Open() {
        kv = new BTreeEngine(PATH, POOL_SIZE);
    }
};

typedef BTreeEngineBaseTest<SIZE> BTreeEngineTest;
typedef BTreeEngineBaseTest<LARGE_SIZE> BTreeEngineLargeTest;


TEST_F(BTreeEngineTest, SimpleTest) {
    string value;
    ASSERT_TRUE(kv->Get("key1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Put("key1", "value1") == OK);
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");
}

TEST_F(BTreeEngineTest, BinaryKeyTest) {
    ASSERT_TRUE(kv->Put("a", "should_not_change") == OK) << pmemobj_errormsg();
    string key1 = string("a\0b", 3);
    ASSERT_TRUE(kv->Put(key1, "stuff") == OK) << pmemobj_errormsg();
    string value;
    ASSERT_TRUE(kv->Get(key1, &value) == OK);
    ASSERT_EQ(value, "stuff");
    string value2;
    ASSERT_TRUE(kv->Get("a", &value2) == OK);
    ASSERT_EQ(value2, "should_not_change");
}

TEST_F(BTreeEngineTest, BinaryValueTest) {
    ASSERT_TRUE(kv->Put("key1", string("A\0B\0\0C", 3)) == OK) << pmemobj_errormsg();
    string value;
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "A\0B\0\0C");
}

TEST_F(BTreeEngineTest, EmptyKeyTest) {
    ASSERT_TRUE(kv->Put("", "empty") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put(" ", "single-space") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("\t\t", "two-tab") == OK) << pmemobj_errormsg();
    string value1;
    string value2;
    string value3;
    ASSERT_TRUE(kv->Get("", &value1) == OK && value1 == "empty");
    ASSERT_TRUE(kv->Get(" ", &value2) == OK && value2 == "single-space");
    ASSERT_TRUE(kv->Get("\t\t", &value3) == OK && value3 == "two-tab");
}

TEST_F(BTreeEngineTest, EmptyValueTest) {
    ASSERT_TRUE(kv->Put("empty", "") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("single-space", " ") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("two-tab", "\t\t") == OK) << pmemobj_errormsg();
    string value1;
    string value2;
    string value3;
    ASSERT_TRUE(kv->Get("empty", &value1) == OK && value1 == "");
    ASSERT_TRUE(kv->Get("single-space", &value2) == OK && value2 == " ");
    ASSERT_TRUE(kv->Get("two-tab", &value3) == OK && value3 == "\t\t");
}

TEST_F(BTreeEngineTest, GetAppendToExternalValueTest) {
    ASSERT_TRUE(kv->Put("key1", "cool") == OK) << pmemobj_errormsg();
    string value = "super";
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "supercool");
}

TEST_F(BTreeEngineTest, GetHeadlessTest) {
    string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(BTreeEngineTest, GetMultipleTest) {
    ASSERT_TRUE(kv->Put("abc", "A1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("def", "B2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("hij", "C3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("jkl", "D4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("mno", "E5") == OK) << pmemobj_errormsg();
    string value1;
    ASSERT_TRUE(kv->Get("abc", &value1) == OK && value1 == "A1");
    string value2;
    ASSERT_TRUE(kv->Get("def", &value2) == OK && value2 == "B2");
    string value3;
    ASSERT_TRUE(kv->Get("hij", &value3) == OK && value3 == "C3");
    string value4;
    ASSERT_TRUE(kv->Get("jkl", &value4) == OK && value4 == "D4");
    string value5;
    ASSERT_TRUE(kv->Get("mno", &value5) == OK && value5 == "E5");
}

TEST_F(BTreeEngineTest, GetMultiple2Test) {
    // TODO: enable this test when remove operation is implemented in versiond B tree
    /*ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key2", "value2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key3", "value3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("key2") == OK);
    ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK) << pmemobj_errormsg();
    string value1;
    ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
    string value2;
    ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
    string value3;
    ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");*/
}

TEST_F(BTreeEngineTest, GetNonexistentTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(BTreeEngineTest, PutTest) {
    string value;
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");

    string new_value;
    ASSERT_TRUE(kv->Put("key1", "VALUE1") == OK) << pmemobj_errormsg();           // same size
    ASSERT_TRUE(kv->Get("key1", &new_value) == OK && new_value == "VALUE1");

    string new_value2;
    ASSERT_TRUE(kv->Put("key1", "new_value") == OK) << pmemobj_errormsg();        // longer size
    ASSERT_TRUE(kv->Get("key1", &new_value2) == OK && new_value2 == "new_value");

    string new_value3;
    ASSERT_TRUE(kv->Put("key1", "?") == OK) << pmemobj_errormsg();                // shorter size
    ASSERT_TRUE(kv->Get("key1", &new_value3) == OK && new_value3 == "?");
}

TEST_F(BTreeEngineTest, PutKeysOfDifferentSizesTest) {
    string value;
    ASSERT_TRUE(kv->Put("123456789ABCDE", "A") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("123456789ABCDE", &value) == OK && value == "A");

    string value2;
    ASSERT_TRUE(kv->Put("123456789ABCDEF", "B") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("123456789ABCDEF", &value2) == OK && value2 == "B");

    string value3;
    ASSERT_TRUE(kv->Put("12345678ABCDEFG", "C") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("12345678ABCDEFG", &value3) == OK && value3 == "C");

    string value4;
    ASSERT_TRUE(kv->Put("123456789", "D") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("123456789", &value4) == OK && value4 == "D");

    string value5;
    ASSERT_TRUE(kv->Put("123456789ABCDEFGHI", "E") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("123456789ABCDEFGHI", &value5) == OK && value5 == "E");
}

TEST_F(BTreeEngineTest, PutValuesOfDifferentSizesTest) {
    string value;
    ASSERT_TRUE(kv->Put("A", "123456789ABCDE") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("A", &value) == OK && value == "123456789ABCDE");

    string value2;
    ASSERT_TRUE(kv->Put("B", "123456789ABCDEF") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("B", &value2) == OK && value2 == "123456789ABCDEF");

    string value3;
    ASSERT_TRUE(kv->Put("C", "12345678ABCDEFG") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("C", &value3) == OK && value3 == "12345678ABCDEFG");

    string value4;
    ASSERT_TRUE(kv->Put("D", "123456789") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("D", &value4) == OK && value4 == "123456789");

    string value5;
    ASSERT_TRUE(kv->Put("E", "123456789ABCDEFGHI") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("E", &value5) == OK && value5 == "123456789ABCDEFGHI");
}

TEST_F(BTreeEngineTest, PutValuesOfMaximumSizeTest) {
    // todo finish this when max is decided (#61)
}

// TODO: enable this test when remove operaation is implemented in versioned B+tree
/*
TEST_F(BTreeEngineTest, RemoveAllTest) {
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
}

TEST_F(BTreeEngineTest, RemoveAndInsertTest) {
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == OK && value == "tmpvalue1");
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}

TEST_F(BTreeEngineTest, RemoveExistingTest) {
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("tmpkey2", "tmpvalue2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK);
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK); // ok to remove twice
    string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Get("tmpkey2", &value) == OK && value == "tmpvalue2");
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}

TEST_F(BTreeEngineTest, RemoveHeadlessTest) {
    ASSERT_TRUE(kv->Remove("nada") == OK);
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 0);
}

TEST_F(BTreeEngineTest, RemoveNonexistentTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("nada") == OK);
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}*/

// =============================================================================================
// TEST RECOVERY OF SINGLE-LEAF TREE
// =============================================================================================

TEST_F(BTreeEngineTest, GetHeadlessAfterRecoveryTest) {
    Reopen();
    string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(BTreeEngineTest, GetMultipleAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("abc", "A1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("def", "B2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("hij", "C3") == OK) << pmemobj_errormsg();
    Reopen();
    ASSERT_TRUE(kv->Put("jkl", "D4") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("mno", "E5") == OK) << pmemobj_errormsg();
    string value1;
    ASSERT_TRUE(kv->Get("abc", &value1) == OK && value1 == "A1");
    string value2;
    ASSERT_TRUE(kv->Get("def", &value2) == OK && value2 == "B2");
    string value3;
    ASSERT_TRUE(kv->Get("hij", &value3) == OK && value3 == "C3");
    string value4;
    ASSERT_TRUE(kv->Get("jkl", &value4) == OK && value4 == "D4");
    string value5;
    ASSERT_TRUE(kv->Get("mno", &value5) == OK && value5 == "E5");
}

// TODO: enable this test when remove operaation is implemented in versioned B+tree
/*
TEST_F(BTreeEngineTest, GetMultiple2AfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key2", "value2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("key3", "value3") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("key2") == OK);
    ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK) << pmemobj_errormsg();
    Reopen();
    string value1;
    ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
    string value2;
    ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
    string value3;
    ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");
}*/

TEST_F(BTreeEngineTest, GetNonexistentAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    Reopen();
    string value;
    ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(BTreeEngineTest, PutAfterRecoveryTest) {
    string value;
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");

    string new_value;
    ASSERT_TRUE(kv->Put("key1", "VALUE1") == OK) << pmemobj_errormsg();           // same size
    ASSERT_TRUE(kv->Get("key1", &new_value) == OK && new_value == "VALUE1");
    Reopen();

    string new_value2;
    ASSERT_TRUE(kv->Put("key1", "new_value") == OK) << pmemobj_errormsg();        // longer size
    ASSERT_TRUE(kv->Get("key1", &new_value2) == OK && new_value2 == "new_value");

    string new_value3;
    ASSERT_TRUE(kv->Put("key1", "?") == OK) << pmemobj_errormsg();                // shorter size
    ASSERT_TRUE(kv->Get("key1", &new_value3) == OK && new_value3 == "?");
}

// TODO: enable this test when remove operaation is implemented in versioned B+tree
/*
TEST_F(BTreeEngineTest, RemoveAllAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    Reopen();
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 1);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}

TEST_F(BTreeEngineTest, RemoveAndInsertAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK) << pmemobj_errormsg();
    Reopen();
    ASSERT_TRUE(kv->Remove("tmpkey") == OK);
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == OK && value == "tmpvalue1");
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}

TEST_F(BTreeEngineTest, RemoveExistingAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Put("tmpkey2", "tmpvalue2") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK);
    Reopen();
    ASSERT_TRUE(kv->Remove("tmpkey1") == OK); // ok to remove twice
    string value;
    ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
    ASSERT_TRUE(kv->Get("tmpkey2", &value) == OK && value == "tmpvalue2");
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}

TEST_F(BTreeEngineTest, RemoveHeadlessAfterRecoveryTest) {
    Reopen();
    ASSERT_TRUE(kv->Remove("nada") == OK);
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 0);
}

TEST_F(BTreeEngineTest, RemoveNonexistentAfterRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    Reopen();
    ASSERT_TRUE(kv->Remove("nada") == OK);
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}

TEST_F(BTreeEngineTest, UsePreallocAfterSingleLeafRecoveryTest) {
    ASSERT_TRUE(kv->Put("key1", "value1") == OK) << pmemobj_errormsg();
    ASSERT_TRUE(kv->Remove("key1") == OK);
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 1);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);

    Reopen();
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 1);
    ASSERT_EQ(analysis.leaf_prealloc, 1);
    ASSERT_EQ(analysis.leaf_total, 1);

    ASSERT_TRUE(kv->Put("key2", "value2") == OK) << pmemobj_errormsg();
    Analyze();
    ASSERT_EQ(analysis.leaf_empty, 0);
    ASSERT_EQ(analysis.leaf_prealloc, 0);
    ASSERT_EQ(analysis.leaf_total, 1);
}*/

// =============================================================================================
// TEST TREE WITH SINGLE INNER NODE
// =============================================================================================

const size_t INNER_ENTRIES = DEGREE - 1;
const size_t LEAF_ENTRIES = DEGREE - 1;
const size_t SINGLE_INNER_LIMIT = LEAF_ENTRIES * (INNER_ENTRIES - 1);

TEST_F(BTreeEngineTest, SingleInnerNodeAscendingTest) {
    for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

TEST_F(BTreeEngineTest, SingleInnerNodeAscendingTest2) {
    for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

TEST_F(BTreeEngineTest, SingleInnerNodeDescendingTest) {
    for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

TEST_F(BTreeEngineTest, SingleInnerNodeDescendingTest2) {
    for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
    for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

// =============================================================================================
// TEST RECOVERY OF TREE WITH SINGLE INNER NODE
// =============================================================================================

TEST_F(BTreeEngineTest, SingleInnerNodeAscendingAfterRecoveryTest) {
    for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Reopen();
    for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

TEST_F(BTreeEngineTest, SingleInnerNodeAscendingAfterRecoveryTest2) {
    for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Reopen();
    for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

TEST_F(BTreeEngineTest, SingleInnerNodeDescendingAfterRecoveryTest) {
    for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Reopen();
    for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

TEST_F(BTreeEngineTest, SingleInnerNodeDescendingAfterRecoveryTest2) {
    for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, istr) == OK) << pmemobj_errormsg();
    }
    Reopen();
    for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == istr);
    }
}

// TODO: enable this test when remove operation is implemented in versiond B tree
/*
TEST_F(BTreeEngineTest, UsePreallocAfterMultipleLeafRecoveryTest) {
    for (int i = 1; i <= LEAF_ENTRIES + 1; i++)
        ASSERT_EQ(kv->Put(to_string(i), "!"), OK) << pmemobj_errormsg();
    Reopen();

    for (int i = 1; i <= LEAF_ENTRIES; i++) ASSERT_EQ(kv->Remove(to_string(i)), OK);
    Reopen();

    ASSERT_EQ(kv->Remove(to_string(LEAF_ENTRIES + 1)), OK);
    Reopen();

    for (int i = 1; i <= LEAF_ENTRIES; i++)
        ASSERT_EQ(kv->Put(to_string(i), "!"), OK) << pmemobj_errormsg();
    ASSERT_EQ(kv->Put(to_string(LEAF_ENTRIES + 1), "!"), OK) << pmemobj_errormsg();
}*/

// =============================================================================================
// TEST LARGE TREE
// =============================================================================================

const int LARGE_LIMIT = 4000000;

TEST_F(BTreeEngineLargeTest, LargeAscendingTest) {
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
}

TEST_F(BTreeEngineLargeTest, LargeDescendingTest) {
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
}

// =============================================================================================
// TEST RECOVERY OF LARGE TREE
// =============================================================================================

TEST_F(BTreeEngineLargeTest, LargeAscendingAfterRecoveryTest) {
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, (istr + "!")) == OK) << pmemobj_errormsg();
    }
    Reopen();
    for (int i = 1; i <= LARGE_LIMIT; i++) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == (istr + "!"));
    }
}

TEST_F(BTreeEngineLargeTest, LargeDescendingAfterRecoveryTest) {
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        ASSERT_TRUE(kv->Put(istr, ("ABC" + istr)) == OK) << pmemobj_errormsg();
    }
    Reopen();
    for (int i = LARGE_LIMIT; i >= 1; i--) {
        string istr = to_string(i);
        string value;
        ASSERT_TRUE(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
    }
}
