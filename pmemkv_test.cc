/*
 * Copyright 2017, Intel Corporation
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

#include "pmemkv.h"
#include "gtest/gtest.h"

#define sizeof_field(type, field) sizeof(((type *)0)->field)

using namespace pmemkv;

const std::string PATH = "/dev/shm/pmemkv";

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class KVTest : public testing::Test {
public:
  KVTree* kv;

  KVTest() {
    std::remove(PATH.c_str());
    Open();
  }

  ~KVTest() { delete kv; }

  void Reopen() {
    delete kv;
    Open();
  }

private:
  void Open() {
    kv = new KVTree(PATH);
    assert(kv->GetName() == PATH);
  }
};

// =============================================================================================
// TEST SINGLE-LEAF TREE
// =============================================================================================

TEST_F(KVTest, SizeofTest) {
  // persistent types
  ASSERT_TRUE(sizeof(KVRoot) == 32);
  ASSERT_TRUE(sizeof(KVLeaf) == 3136);
  ASSERT_TRUE(sizeof_field(KVLeaf, hashes) + sizeof_field(KVLeaf, next) == 64);
  ASSERT_TRUE(sizeof(KVString) == 32);

  // volatile types
  ASSERT_TRUE(sizeof(KVInnerNode) == 232);
  ASSERT_TRUE(sizeof(KVLeafNode) == 88);
}

TEST_F(KVTest, DeleteAllTest) {
  ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK);
  ASSERT_TRUE(kv->Delete("tmpkey") == OK);
  ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK);
  std::string value;
  ASSERT_TRUE(kv->Get("tmpkey1", &value) == OK && value == "tmpvalue1");
}

TEST_F(KVTest, DeleteExistingTest) {
  ASSERT_TRUE(kv->Put("tmpkey1", "tmpvalue1") == OK);
  ASSERT_TRUE(kv->Put("tmpkey2", "tmpvalue2") == OK);
  ASSERT_TRUE(kv->Delete("tmpkey1") == OK);
  ASSERT_TRUE(kv->Delete("tmpkey1") == OK); // ok to delete twice
  std::string value;
  ASSERT_TRUE(kv->Get("tmpkey1", &value) == NOT_FOUND);
  ASSERT_TRUE(kv->Get("tmpkey2", &value) == OK && value == "tmpvalue2");
}

TEST_F(KVTest, DeleteHeadlessTest) {
  ASSERT_TRUE(kv->Delete("nada") == OK);
}

TEST_F(KVTest, DeleteNonexistentTest) {
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  ASSERT_TRUE(kv->Delete("nada") == OK);
}

TEST_F(KVTest, EmptyKeyTest) {                                      // todo correct behavior?
  ASSERT_TRUE(kv->Put("", "blah") == OK);
  std::string value;
  ASSERT_TRUE(kv->Get("", &value) == OK && value == "blah");
}

TEST_F(KVTest, EmptyValueTest) {                                    // todo correct behavior?
  ASSERT_TRUE(kv->Put("key1", "") == OK);
  std::string value;
  ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "");
}

TEST_F(KVTest, GetAppendToExternalValueTest) {
  ASSERT_TRUE(kv->Put("key1", "cool") == OK);
  std::string value = "super";
  ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "supercool");
}

TEST_F(KVTest, GetHeadlessTest) {
  std::string value;
  ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(KVTest, GetMultipleTest) {
  ASSERT_TRUE(kv->Put("abc", "A1") == OK);
  ASSERT_TRUE(kv->Put("def", "B2") == OK);
  ASSERT_TRUE(kv->Put("hij", "C3") == OK);
  ASSERT_TRUE(kv->Put("jkl", "D4") == OK);
  ASSERT_TRUE(kv->Put("mno", "E5") == OK);
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

TEST_F(KVTest, GetMultipleAfterDeleteTest) {
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  ASSERT_TRUE(kv->Put("key2", "value2") == OK);
  ASSERT_TRUE(kv->Put("key3", "value3") == OK);
  ASSERT_TRUE(kv->Delete("key2") == OK);
  ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK);
  std::string value1;
  ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
  std::string value2;
  ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
  std::string value3;
  ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");
}

TEST_F(KVTest, GetNonexistentTest) {
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  std::string value;
  ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(KVTest, MultiGetTest) {
  ASSERT_TRUE(kv->Put("tmpkey", "tmpvalue1") == OK);
  ASSERT_TRUE(kv->Put("tmpkey2", "tmpvalue2") == OK);
  auto values = std::vector<std::string>();
  auto keys = std::vector<std::string>();
  keys.push_back("tmpkey");
  keys.push_back("tmpkey2");
  keys.push_back("tmpkey3");
  keys.push_back("tmpkey");
  std::vector<KVStatus> status = kv->MultiGet(keys, &values);
  ASSERT_TRUE(status.size() == 4);
  ASSERT_TRUE(values.size() == 4);
  ASSERT_TRUE(status.at(0) == OK && values.at(0) == "tmpvalue1");
  ASSERT_TRUE(status.at(1) == OK && values.at(1) == "tmpvalue2");
  ASSERT_TRUE(status.at(2) == NOT_FOUND && values.at(2) == "");
  ASSERT_TRUE(status.at(3) == OK && values.at(3) == "tmpvalue1");
}

TEST_F(KVTest, PutExistingTest) {
  std::string value;
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  ASSERT_TRUE(kv->Get("key1", &value) == OK && value == "value1");

  std::string new_value;
  ASSERT_TRUE(kv->Put("key1", "VALUE1") == OK);           // same length
  ASSERT_TRUE(kv->Get("key1", &new_value) == OK && new_value == "VALUE1");

  std::string new_value2;
  ASSERT_TRUE(kv->Put("key1", "new_value") == OK);        // longer length
  ASSERT_TRUE(kv->Get("key1", &new_value2) == OK && new_value2 == "new_value");

  std::string new_value3;
  ASSERT_TRUE(kv->Put("key1", "?") == OK);                // shorter length
  ASSERT_TRUE(kv->Get("key1", &new_value3) == OK && new_value3 == "?");
}

TEST_F(KVTest, PutKeysOfDifferentLengthsTest) {
  std::string value;
  ASSERT_TRUE(kv->Put("123456789ABCDE", "A") == OK);      // 2 under the sso limit
  ASSERT_TRUE(kv->Get("123456789ABCDE", &value) == OK && value == "A");

  std::string value2;
  ASSERT_TRUE(kv->Put("123456789ABCDEF", "B") == OK);     // 1 under the sso limit
  ASSERT_TRUE(kv->Get("123456789ABCDEF", &value2) == OK && value2 == "B");

  std::string value3;
  ASSERT_TRUE(kv->Put("123456789ABCDEFG", "C") == OK);    // at the sso limit
  ASSERT_TRUE(kv->Get("123456789ABCDEFG", &value3) == OK && value3 == "C");

  std::string value4;
  ASSERT_TRUE(kv->Put("123456789ABCDEFGH", "D") == OK);   // 1 over the sso limit
  ASSERT_TRUE(kv->Get("123456789ABCDEFGH", &value4) == OK && value4 == "D");

  std::string value5;
  ASSERT_TRUE(kv->Put("123456789ABCDEFGHI", "E") == OK);   // 2 over the sso limit
  ASSERT_TRUE(kv->Get("123456789ABCDEFGHI", &value5) == OK && value5 == "E");
}

TEST_F(KVTest, PutValuesOfDifferentLengthsTest) {
  std::string value;
  ASSERT_TRUE(kv->Put("A", "123456789ABCDE") == OK);      // 2 under the sso limit
  ASSERT_TRUE(kv->Get("A", &value) == OK && value == "123456789ABCDE");

  std::string value2;
  ASSERT_TRUE(kv->Put("B", "123456789ABCDEF") == OK);     // 1 under the sso limit
  ASSERT_TRUE(kv->Get("B", &value2) == OK && value2 == "123456789ABCDEF");

  std::string value3;
  ASSERT_TRUE(kv->Put("C", "123456789ABCDEFG") == OK);    // at the sso limit
  ASSERT_TRUE(kv->Get("C", &value3) == OK && value3 == "123456789ABCDEFG");

  std::string value4;
  ASSERT_TRUE(kv->Put("D", "123456789ABCDEFGH") == OK);   // 1 over the sso limit
  ASSERT_TRUE(kv->Get("D", &value4) == OK && value4 == "123456789ABCDEFGH");

  std::string value5;
  ASSERT_TRUE(kv->Put("E", "123456789ABCDEFGHI") == OK);  // 2 over the sso limit
  ASSERT_TRUE(kv->Get("E", &value5) == OK && value5 == "123456789ABCDEFGHI");
}

// =============================================================================================
// TEST RECOVERY OF SINGLE-LEAF TREE
// =============================================================================================

TEST_F(KVTest, DeleteHeadlessAfterRecoveryTest) {
  Reopen();
  ASSERT_TRUE(kv->Delete("nada") == OK);
}

TEST_F(KVTest, DeleteNonexistentAfterRecoveryTest) {
  Reopen();
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  ASSERT_TRUE(kv->Delete("nada") == OK);
}

TEST_F(KVTest, GetHeadlessAfterRecoveryTest) {
  Reopen();
  std::string value;
  ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(KVTest, GetMultipleAfterRecoveryTest) {
  ASSERT_TRUE(kv->Put("abc", "A1") == OK);
  ASSERT_TRUE(kv->Put("def", "B2") == OK);
  ASSERT_TRUE(kv->Put("hij", "C3") == OK);
  Reopen();
  ASSERT_TRUE(kv->Put("jkl", "D4") == OK);
  ASSERT_TRUE(kv->Put("mno", "E5") == OK);
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

TEST_F(KVTest, GetNonexistentAfterRecoveryTest) {
  Reopen();
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  std::string value;
  ASSERT_TRUE(kv->Get("waldo", &value) == NOT_FOUND);
}

TEST_F(KVTest, PutAfterRecoveryTest) {
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  Reopen();
  std::string value1;
  ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
}

TEST_F(KVTest, UpdateAfterRecoveryTest) {
  ASSERT_TRUE(kv->Put("key1", "value1") == OK);
  ASSERT_TRUE(kv->Put("key2", "value2") == OK);
  ASSERT_TRUE(kv->Put("key3", "value3") == OK);
  ASSERT_TRUE(kv->Delete("key2") == OK);
  ASSERT_TRUE(kv->Put("key3", "VALUE3") == OK);
  Reopen();
  std::string value1;
  ASSERT_TRUE(kv->Get("key1", &value1) == OK && value1 == "value1");
  std::string value2;
  ASSERT_TRUE(kv->Get("key2", &value2) == NOT_FOUND);
  std::string value3;
  ASSERT_TRUE(kv->Get("key3", &value3) == OK && value3 == "VALUE3");
}

// =============================================================================================
// TEST TREE WITH SINGLE INNER NODE
// =============================================================================================

const int SINGLE_INNER_LIMIT = NODE_KEYS * (INNER_KEYS - 1);

TEST_F(KVTest, SingleInnerNodeAscendingTest) {
  for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
  for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

TEST_F(KVTest, SingleInnerNodeAscendingTest2) {
  for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
  for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

TEST_F(KVTest, SingleInnerNodeDescendingTest) {
  for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
  for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

TEST_F(KVTest, SingleInnerNodeDescendingTest2) {
  for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
  for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

// =============================================================================================
// TEST RECOVERY OF TREE WITH SINGLE INNER NODE
// =============================================================================================

TEST_F(KVTest, SingleInnerNodeAscendingAfterRecoveryTest) {
  for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
  }
  Reopen();
  for (int i = 10000; i <= (10000 + SINGLE_INNER_LIMIT); i++) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

TEST_F(KVTest, SingleInnerNodeAscendingAfterRecoveryTest2) {
  for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
  }
  Reopen();
  for (int i = 1; i <= SINGLE_INNER_LIMIT; i++) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

TEST_F(KVTest, SingleInnerNodeDescendingAfterRecoveryTest) {
  for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
  }
  Reopen();
  for (int i = (10000 + SINGLE_INNER_LIMIT); i >= 10000; i--) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

TEST_F(KVTest, SingleInnerNodeDescendingAfterRecoveryTest2) {
  for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, istr) == OK);
  }
  Reopen();
  for (int i = SINGLE_INNER_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == istr);
  }
}

// =============================================================================================
// TEST LARGE TREE
// =============================================================================================

const int LARGE_LIMIT = 1000000;

TEST_F(KVTest, LargeAscendingTest) {
  for (int i = 1; i <= LARGE_LIMIT; i++) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, (istr + "!")) == OK);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == (istr + "!"));
  }
  for (int i = 1; i <= LARGE_LIMIT; i++) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == (istr + "!"));
  }
}

TEST_F(KVTest, LargeDescendingTest) {
  for (int i = LARGE_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, ("ABC" + istr)) == OK);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
  }
  for (int i = LARGE_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
  }
}

// =============================================================================================
// TEST RECOVERY OF LARGE TREE
// =============================================================================================

TEST_F(KVTest, LargeAscendingAfterRecoveryTest) {
  for (int i = 1; i <= LARGE_LIMIT; i++) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, (istr + "!")) == OK);
  }
  Reopen();
  for (int i = 1; i <= LARGE_LIMIT; i++) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == (istr + "!"));
  }
}

TEST_F(KVTest, LargeDescendingAfterRecoveryTest) {
  for (int i = LARGE_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    assert(kv->Put(istr, ("ABC" + istr)) == OK);
  }
  Reopen();
  for (int i = LARGE_LIMIT; i >= 1; i--) {
    std::string istr = std::to_string(i);
    std::string value;
    assert(kv->Get(istr, &value) == OK && value == ("ABC" + istr));
  }
}
