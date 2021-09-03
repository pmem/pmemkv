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

static void test_put_abort(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin().get_value();

	auto proto = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, tx);
	verify_not_found(proto, kv);
	ASSERT_SIZE(kv, 0);

	tx.abort();

	verify_not_found(proto, kv);
}

static void test_put_commit(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin().get_value();

	auto proto = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, tx);
	verify_not_found(proto, kv);
	ASSERT_SIZE(kv, 0);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	VerifyKv(proto, kv);
}

static void test_put_destroy(pmem::kv::db &kv)
{
	std::map<std::string, std::string> proto;

	{
		auto tx = kv.tx_begin().get_value();
		proto = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, tx);
		verify_not_found(proto, kv);
		ASSERT_SIZE(kv, 0);
	}

	verify_not_found(proto, kv);
	ASSERT_SIZE(kv, 0);
}

static void test_overwrite_commit(pmem::kv::db &kv)
{
	/* Initialize kv */
	auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	auto tx = kv.tx_begin().get_value();
	auto proto_tx = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH * 2, tx);

	VerifyKv(proto_kv, kv);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	VerifyKv(proto_tx, kv);
}

static void test_overwrite_abort(pmem::kv::db &kv)
{
	/* Initialize kv */
	auto proto_kv = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, kv);

	auto tx = kv.tx_begin().get_value();
	auto proto_tx = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH * 2, tx);

	VerifyKv(proto_kv, kv);

	tx.abort();

	VerifyKv(proto_kv, kv);
}

static void test_use_after_commit(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin().get_value();
	auto proto = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, tx);

	verify_not_found(proto, kv);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	/* Rollback changes */
	for (auto &e : proto)
		ASSERT_STATUS(kv.remove(e.first), pmem::kv::status::OK);

	ASSERT_STATUS(tx.put("extra_key", "extra_value"), pmem::kv::status::OK);
	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	verify_not_found(proto, kv);
	ASSERT_STATUS(kv.exists("extra_key"), pmem::kv::status::OK);
	ASSERT_SIZE(kv, 1);
}

static void test_use_after_abort(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin().get_value();
	auto proto = PutToMapTest(N_INSERTS, KEY_LENGTH, VALUE_LENGTH, tx);

	verify_not_found(proto, kv);

	tx.abort();

	ASSERT_STATUS(tx.put("extra_key", "extra_value"), pmem::kv::status::OK);
	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);

	verify_not_found(proto, kv);
	ASSERT_STATUS(kv.exists("extra_key"), pmem::kv::status::OK);
	ASSERT_SIZE(kv, 1);
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

	ASSERT_SIZE(kv, NUM_BATCH * BATCH_SIZE);

	for (int i = 0; i < NUM_BATCH; i++) {
		for (int j = 0; j < BATCH_SIZE; j++) {
			std::string key = gen_key(i, j);
			std::string val;
			ASSERT_STATUS(kv.get(key, &val), pmem::kv::status::OK);
			UT_ASSERT(val == key);
		}
	}
}

// XXX - add tests with UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle()))); once
// destroy() is implemented

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {test_put_abort, test_put_commit, test_put_destroy,
			  test_batched_updates, test_use_after_commit,
			  test_use_after_abort, test_overwrite_abort,
			  test_overwrite_commit});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
