// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "../put_get_std_map.hpp"
#include "unittest.hpp"

const size_t N_INSERTS = 10;
const size_t KEY_LENGTH = 10;
const size_t VALUE_LENGTH = 10;

static void verify_not_found(const std::map<std::string, std::string> &elements,
			     pmem::kv::db &kv)
{
	for (auto &e : elements)
		ASSERT_STATUS(kv.exists(e.first), pmem::kv::status::NOT_FOUND);
}

static void test_remove_commit(pmem::kv::db &kv)
{
	auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	auto tx = kv.tx_begin().get_value();
	for (auto &e : proto_kv)
		ASSERT_STATUS(tx.remove(e.first), pmem::kv::status::OK);

	VerifyKv(proto_kv, kv);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	verify_not_found(proto_kv, kv);
}

static void test_remove_abort(pmem::kv::db &kv)
{
	auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	auto tx = kv.tx_begin().get_value();
	for (auto &e : proto_kv)
		ASSERT_STATUS(tx.remove(e.first), pmem::kv::status::OK);

	VerifyKv(proto_kv, kv);

	tx.abort();

	VerifyKv(proto_kv, kv);
}

static void test_remove_destroy(pmem::kv::db &kv)
{
	std::map<std::string, std::string> proto_kv;

	{
		proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

		auto tx = kv.tx_begin().get_value();
		for (auto &e : proto_kv)
			ASSERT_STATUS(tx.remove(e.first), pmem::kv::status::OK);

		VerifyKv(proto_kv, kv);
	}

	VerifyKv(proto_kv, kv);
}

static void test_remove_inserted(pmem::kv::db &kv)
{
	const int NUM_ITER = 100;

	auto gen_key = [](int i) { return "unique_prefix" + std::to_string(i); };

	/* remove each inserted element */
	{
		auto tx = kv.tx_begin().get_value();

		for (int i = 0; i < NUM_ITER; i++) {
			auto e = gen_key(i);
			ASSERT_STATUS(tx.put(e, e), pmem::kv::status::OK);
			ASSERT_STATUS(tx.remove(e), pmem::kv::status::OK);
		}

		ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

		ASSERT_SIZE(kv, 0);
	}

	/* remove every second inserted element */
	{
		auto tx = kv.tx_begin().get_value();

		for (int i = 0; i < NUM_ITER; i++) {
			auto e = gen_key(i);
			ASSERT_STATUS(tx.put(e, e), pmem::kv::status::OK);
			if (i % 2 == 0)
				ASSERT_STATUS(tx.remove(e), pmem::kv::status::OK);
		}

		ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

		ASSERT_SIZE(kv, NUM_ITER / 2);
	}

	/* remove each inserted element but start with non-empty database */
	{
		auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);
		auto tx = kv.tx_begin().get_value();

		for (int i = 0; i < NUM_ITER; i++) {
			auto e = gen_key(i);
			ASSERT_STATUS(tx.put(e, e), pmem::kv::status::OK);
			ASSERT_STATUS(tx.remove(e), pmem::kv::status::OK);
		}

		ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

		ASSERT_SIZE(kv, proto_kv.size());
	}
}

static void test_put_and_remove(pmem::kv::db &kv)
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
			ASSERT_STATUS(kv.put(key, value), pmem::kv::status::OK);
		}

		/* remove half the elements inserted above and BATCH_SIZE of non-existent
		 * elements (should have no effect) */
		for (int j = BATCH_SIZE / 2; j < BATCH_SIZE + BATCH_SIZE / 2; j++) {
			ASSERT_STATUS(tx.remove(gen_key(i, j)), pmem::kv::status::OK);
		}

		ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);
	}

	ASSERT_SIZE(kv, NUM_BATCH * BATCH_SIZE / 2);

	for (int i = 0; i < NUM_BATCH; i++) {
		for (int j = 0; j < BATCH_SIZE / 2; j++) {
			std::string key = gen_key(i, j);
			ASSERT_STATUS(kv.exists(key), pmem::kv::status::OK);
		}
	}
}

static void test_use_after_commit(pmem::kv::db &kv)
{
	auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	auto tx = kv.tx_begin().get_value();
	for (auto &e : proto_kv)
		ASSERT_STATUS(tx.remove(e.first), pmem::kv::status::OK);

	VerifyKv(proto_kv, kv);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	verify_not_found(proto_kv, kv);

	/* Rollback changes */
	PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	ASSERT_STATUS(tx.put("extra_key", "extra_value"), pmem::kv::status::OK);
	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	for (const auto &record : proto_kv) {
		const auto &key = record.first;
		const auto &val = record.second;

		auto s = kv.get(key, [&](string_view value) {
			UT_ASSERT(value.compare(val) == 0);
		});
		ASSERT_STATUS(s, status::OK);
	}
	ASSERT_STATUS(kv.exists("extra_key"), pmem::kv::status::OK);
	ASSERT_SIZE(kv, proto_kv.size() + 1);
}

static void test_use_after_abort(pmem::kv::db &kv)
{
	auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	auto tx = kv.tx_begin().get_value();
	for (auto &e : proto_kv)
		ASSERT_STATUS(tx.remove(e.first), pmem::kv::status::OK);

	VerifyKv(proto_kv, kv);

	tx.abort();

	ASSERT_STATUS(tx.put("extra_key", "extra_value"), pmem::kv::status::OK);
	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	for (const auto &record : proto_kv) {
		const auto &key = record.first;
		const auto &val = record.second;

		auto s = kv.get(key, [&](string_view value) {
			UT_ASSERT(value.compare(val) == 0);
		});
		ASSERT_STATUS(s, status::OK);
	}
	ASSERT_STATUS(kv.exists("extra_key"), pmem::kv::status::OK);
	ASSERT_SIZE(kv, proto_kv.size() + 1);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {test_remove_commit, test_remove_abort, test_remove_destroy,
			  test_remove_inserted, test_put_and_remove,
			  test_use_after_commit, test_use_after_abort});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
