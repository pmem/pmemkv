// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static void test_abort(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin();
	tx.put("a", "a");
	tx.put("b", "b");
	tx.put("c", "c");

	ASSERT_STATUS(kv.exists("a"), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("b"), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("c"), pmem::kv::status::NOT_FOUND);

	tx.abort();

	ASSERT_STATUS(kv.exists("a"), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("b"), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("c"), pmem::kv::status::NOT_FOUND);
}

static void test_commit(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin();
	tx.put("a", "a");
	tx.put("b", "b");
	tx.put("c", "c");

	ASSERT_STATUS(kv.exists("a"), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("b"), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(kv.exists("c"), pmem::kv::status::NOT_FOUND);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	ASSERT_STATUS(kv.exists("a"), pmem::kv::status::OK);
	ASSERT_STATUS(kv.exists("b"), pmem::kv::status::OK);
	ASSERT_STATUS(kv.exists("c"), pmem::kv::status::OK);
}

static void test_batched_updates(pmem::kv::db &kv)
{
	const int NUM_BATCH = 10000;
	const int BATCH_SIZE = 10;

	auto gen_key = [](int b, int i) {
		return std::to_string(b) + ";" + std::to_string(i) + std::string(40, 'X');
	};

	for (int i = 0; i < NUM_BATCH; i++) {
		auto tx = kv.tx_begin();

		for (int j = 0; j < BATCH_SIZE; j++) {
			std::string key = gen_key(i, j);
			std::string value = key;
			ASSERT_STATUS(tx.put(key, value), pmem::kv::status::OK);
			ASSERT_STATUS(kv.exists(key), pmem::kv::status::NOT_FOUND);
		}

		ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);
	}

	for (int i = 0; i < NUM_BATCH; i++) {
		for (int j = 0; j < BATCH_SIZE; j++) {
			std::string key = gen_key(i, j);
			ASSERT_STATUS(kv.exists(key), pmem::kv::status::OK);
		}
	}
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {test_abort, test_commit, test_batched_updates});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
