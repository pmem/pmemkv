// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static std::vector<std::string> elements = {"A", "B", "C"};

static void test_put_abort(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin().get_value();
	for (auto &e : elements)
		ASSERT_STATUS(tx.put(e, e), pmem::kv::status::OK);

	for (auto &e : elements)
		ASSERT_STATUS(kv.exists(e), pmem::kv::status::NOT_FOUND);

	tx.abort();

	for (auto &e : elements)
		ASSERT_STATUS(kv.exists(e), pmem::kv::status::NOT_FOUND);
}

static void test_put_commit(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin().get_value();
	for (auto &e : elements)
		ASSERT_STATUS(tx.put(e, e), pmem::kv::status::OK);

	for (auto &e : elements)
		ASSERT_STATUS(kv.exists(e), pmem::kv::status::NOT_FOUND);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	for (auto &e : elements)
		ASSERT_STATUS(kv.exists(e), pmem::kv::status::OK);
}

static void test_batched_updates(pmem::kv::db &kv)
{
	const int NUM_BATCH = 10000;
	const int BATCH_SIZE = 10;

	auto gen_key = [](int b, int i) {
		return std::to_string(b) + ";" + std::to_string(i) + std::string(40, 'X');
	};

	for (int i = 0; i < NUM_BATCH; i++) {
		auto tx = kv.tx_begin().get_value();

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
			 {test_put_abort, test_put_commit, test_batched_updates});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
