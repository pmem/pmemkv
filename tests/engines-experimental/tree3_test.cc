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

#include "test_env.hpp"
#include "unittest.hpp"

#include "../../src/engines-experimental/tree3.h"

using namespace pmem::kv;

const int SINGLE_INNER_LIMIT = LEAF_KEYS * (INNER_KEYS - 1);

static void TreeSingleInnerNodeAscendingTest(std::unique_ptr<TestEnv> env)
{
	auto kv = initialize_kv(env->engine, env->get_config());

	for (std::size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
		std::string istr = std::to_string(i);
		UT_ASSERT(kv.put(istr, istr) == status::OK);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	for (std::size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
		std::string istr = std::to_string(i);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

	teardown_kv(std::move(kv));
}

static void TreeSingleInnerNodeAscendingTest2(std::unique_ptr<TestEnv> env)
{
	auto kv = initialize_kv(env->engine, env->get_config());

	for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
		std::string istr = std::to_string(i);
		UT_ASSERT(kv.put(istr, istr) == status::OK);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
		std::string istr = std::to_string(i);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

	teardown_kv(std::move(kv));
}

static void TreeSingleInnerNodeDescendingTest(std::unique_ptr<TestEnv> env)
{
	auto kv = initialize_kv(env->engine, env->get_config());

	for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
		std::string istr = std::to_string(i);
		UT_ASSERT(kv.put(istr, istr) == status::OK);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

	teardown_kv(std::move(kv));
}

static void TreeSingleInnerNodeDescendingTest2(std::unique_ptr<TestEnv> env)
{
	auto kv = initialize_kv(env->engine, env->get_config());

	for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
		std::string istr = std::to_string(i);
		UT_ASSERT(kv.put(istr, istr) == status::OK);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

	teardown_kv(std::move(kv));
}

// =============================================================================================
// TEST RECOVERY OF TREE WITH SINGLE INNER NODE
// =============================================================================================

static void TreeSingleInnerNodeAscendingAfterRecoveryTest(std::unique_ptr<TestEnv> env)
{
	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, istr) == status::OK);
		}

		kv.close();
	}

	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (size_t i = 10000; i < (10000 + SINGLE_INNER_LIMIT); i++) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
		}
		std::size_t cnt = std::numeric_limits<std::size_t>::max();
		UT_ASSERT(kv.count_all(cnt) == status::OK);
		UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

		teardown_kv(std::move(kv));
	}
}

static void TreeSingleInnerNodeAscendingAfterRecoveryTest2(std::unique_ptr<TestEnv> env)
{
	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, istr) == status::OK);
		}

		kv.close();
	}

	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (size_t i = 0; i < SINGLE_INNER_LIMIT; i++) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
		}
		std::size_t cnt = std::numeric_limits<std::size_t>::max();
		UT_ASSERT(kv.count_all(cnt) == status::OK);
		UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

		teardown_kv(std::move(kv));
	}
}

static void TreeSingleInnerNodeDescendingAfterRecoveryTest(std::unique_ptr<TestEnv> env)
{
	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, istr) == status::OK);
		}

		kv.close();
	}

	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (int i = (10000 + SINGLE_INNER_LIMIT); i > 10000; i--) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
		}
		std::size_t cnt = std::numeric_limits<std::size_t>::max();
		UT_ASSERT(kv.count_all(cnt) == status::OK);
		UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

		teardown_kv(std::move(kv));
	}
}

static void TreeSingleInnerNodeDescendingAfterRecoveryTest2(std::unique_ptr<TestEnv> env)
{
	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, istr) == status::OK);
		}

		kv.close();
	}

	{
		auto kv = initialize_kv(env->engine, env->get_config());

		for (int i = SINGLE_INNER_LIMIT; i > 0; i--) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK && value == istr);
		}
		std::size_t cnt = std::numeric_limits<std::size_t>::max();
		UT_ASSERT(kv.count_all(cnt) == status::OK);
		UT_ASSERT(cnt == SINGLE_INNER_LIMIT);

		teardown_kv(std::move(kv));
	}
}

/* XXX
//
=============================================================================================
// TEST RUNNING OUT OF SPACE
//
=============================================================================================

class TreeFullTest : public testing::Test {
public:
	std::string PATH = test_path + "/tree3_full_test";
	const std::string PATH_CACHED = test_path + "/tree3_full_cached_test";

	db *kv;

	TreeFullTest()
	{
		std::remove(PATH.c_str());
		Start();
	}

	~TreeFullTest()
	{
		delete kv;
	}

	void Restart()
	{
		delete kv;
		kv = new db;
		auto s = kv.open("tree3", getConfig(PATH, SIZE, true));
		if (s != status::OK)
			throw std::runtime_error(errormsg());
	}

	void Validate()
	{
		for (int i = 1; i <= LARGE_LIMIT; i++) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK &&
				    value == (istr + "!"));
		}

		Restart();

		UT_ASSERT(kv.put("1", "!1") == status::OK);
		std::string value;
		UT_ASSERT(kv.get("1", &value) == status::OK && value == ("!1"));
		UT_ASSERT(kv.put("1", "1!") == status::OK);
		std::string value2;
		UT_ASSERT(kv.get("1", &value2) == status::OK && value2 == ("1!"));

		for (int i = 1; i <= LARGE_LIMIT; i++) {
			std::string istr = std::to_string(i);
			std::string value3;
			UT_ASSERT(kv.get(istr, &value3) == status::OK &&
				    value3 == (istr + "!"));
		}
	}

private:
	void Start()
	{
		if (access(PATH_CACHED.c_str(), F_OK) == 0) {
			UT_ASSERT(std::system(("cp -f " + PATH_CACHED + " " + PATH)
							.c_str()) == 0);
		} else {
			std::cout << "!!! creating cached copy at " << PATH_CACHED
				  << "\n";
			db *kvt = new db;
			auto s = kvt->open("tree3", getConfig(PATH, SIZE));
			if (s != status::OK)
				throw std::runtime_error(errormsg());
			for (int i = 1; i <= LARGE_LIMIT; i++) {
				std::string istr = std::to_string(i);
				UT_ASSERT(kvt->put(istr, (istr + "!")) == status::OK)
					;
			}
			delete kvt;
			UT_ASSERT(std::system(("cp -f " + PATH + " " + PATH_CACHED)
							.c_str()) == 0);
		}
		kv = new db;
		auto s = kv.open("tree3", getConfig(PATH, SIZE));
		if (s != status::OK)
			throw std::runtime_error(errormsg());
	}
};

const std::string LONGSTR =
	"123456789A123456789A123456789A123456789A123456789A123456789A123456789A";

TEST_F(TreeFullTest, OutOfSpace1Test)
{
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put("100", "?") != status::OK);
	tx_alloc_should_fail = false;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace2aTest)
{
	UT_ASSERT(kv.remove("100") == status::OK);
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put("100", LONGSTR) != status::OK);
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.put("100", "100!") == status::OK) ;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace2bTest)
{
	UT_ASSERT(kv.remove("100") == status::OK);
	UT_ASSERT(kv.put("100", "100!") == status::OK) ;
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put("100", LONGSTR) != status::OK);
	tx_alloc_should_fail = false;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace3aTest)
{
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put("100", LONGSTR) != status::OK);
	tx_alloc_should_fail = false;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace3bTest)
{
	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		UT_ASSERT(kv.put("123456", LONGSTR) != status::OK);
	}
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.remove("4567") == status::OK);
	UT_ASSERT(kv.put("4567", "4567!") == status::OK) ;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace4aTest)
{
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put(std::to_string(LARGE_LIMIT + 1), "1") != status::OK);
	tx_alloc_should_fail = false;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace4bTest)
{
	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		UT_ASSERT(kv.put(std::to_string(LARGE_LIMIT + 1), "1") != status::OK);
	}
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.remove("98765") == status::OK);
	UT_ASSERT(kv.put("98765", "98765!") == status::OK) ;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace5aTest)
{
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put(LONGSTR, "1") != status::OK);
	UT_ASSERT(kv.put(LONGSTR, LONGSTR) != status::OK);
	tx_alloc_should_fail = false;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace5bTest)
{
	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		UT_ASSERT(kv.put(LONGSTR, "1") != status::OK);
		UT_ASSERT(kv.put(LONGSTR, LONGSTR) != status::OK);
	}
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.remove("34567") == status::OK);
	UT_ASSERT(kv.put("34567", "34567!") == status::OK) ;
	Validate();
}

TEST_F(TreeFullTest, OutOfSpace6Test)
{
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put(LONGSTR, "?") != status::OK);
	tx_alloc_should_fail = false;
	std::string str;
	UT_ASSERT(kv.get(LONGSTR, &str) == status::NOT_FOUND);
	Validate();
}

TEST_F(TreeFullTest, RepeatedRecoveryTest)
{
	for (int i = 1; i <= 100; i++)
		Restart();
	Validate();
}
*/

int main(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s file-name engine initializer-type", argv[0]);

	return run_engine_tests(
		argv[1], argv[2], argv[3],
		{TreeSingleInnerNodeAscendingTest, TreeSingleInnerNodeAscendingTest2,
		 TreeSingleInnerNodeDescendingTest, TreeSingleInnerNodeDescendingTest2,
		 TreeSingleInnerNodeAscendingAfterRecoveryTest,
		 TreeSingleInnerNodeAscendingAfterRecoveryTest2,
		 TreeSingleInnerNodeDescendingAfterRecoveryTest,
		 TreeSingleInnerNodeDescendingAfterRecoveryTest2});
}
