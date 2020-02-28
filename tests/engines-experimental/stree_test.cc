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

#include "../../src/engines-experimental/stree.h"

using namespace pmem::kv;

const size_t INNER_ENTRIES = internal::stree::DEGREE - 1;
const size_t LEAF_ENTRIES = internal::stree::DEGREE - 1;
const size_t SINGLE_INNER_LIMIT = LEAF_ENTRIES * (INNER_ENTRIES - 1);

static void StreeSingleInnerNodeAscendingTest(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeAscendingTest2(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeDescendingTest(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeDescendingTest2(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeAscendingAfterRecoveryTest(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeAscendingAfterRecoveryTest2(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeDescendingAfterRecoveryTest(std::unique_ptr<TestEnv> env)
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

static void StreeSingleInnerNodeDescendingAfterRecoveryTest2(std::unique_ptr<TestEnv> env)
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

int main(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s file-name engine initializer-type", argv[0]);

	return run_engine_tests(
		argv[1], argv[2], argv[3],
		{StreeSingleInnerNodeAscendingTest, StreeSingleInnerNodeAscendingTest2,
		 StreeSingleInnerNodeDescendingTest, StreeSingleInnerNodeDescendingTest2,
		 StreeSingleInnerNodeAscendingAfterRecoveryTest,
		 StreeSingleInnerNodeAscendingAfterRecoveryTest2,
		 StreeSingleInnerNodeDescendingAfterRecoveryTest,
		 StreeSingleInnerNodeDescendingAfterRecoveryTest2});
}
