// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../all/put_get_std_map.hpp"

const size_t key_length = 10;
const size_t value_length = 10;

static void insert_and_reserve(std::string engine, std::string config, size_t insert,
			       size_t reserve)
{
	auto cfg = CONFIG_FROM_JSON(config);
	auto kv = INITIALIZE_KV(engine, std::move(cfg));

	auto proto = PutToMapTest(insert, key_length, value_length, kv);
	VerifyKv(proto, kv);
	kv.close();

	/* reopen */
	cfg = CONFIG_FROM_JSON(config);
	cfg.put_reserve(reserve);

	kv = INITIALIZE_KV(engine, std::move(cfg));
	VerifyKv(proto, kv);

	/* add few more records */
	auto proto2 = PutToMapTest(5, key_length, value_length, kv);
	VerifyKv(proto, kv);
	VerifyKv(proto2, kv);
	kv.close();
}

static void reserve_and_insert(std::string engine, std::string config, size_t reserve,
			       size_t insert)
{
	auto cfg = CONFIG_FROM_JSON(config);
	cfg.put_reserve(reserve);

	auto kv = INITIALIZE_KV(engine, std::move(cfg));

	auto proto = PutToMapTest(insert, key_length, value_length, kv);
	VerifyKv(proto, kv);

	CLEAR_KV(kv);
	kv.close();
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	/* reserve more elements than inserting */
	reserve_and_insert(argv[1], argv[2], 100, 50);

	/* reserve the same count */
	reserve_and_insert(argv[1], argv[2], 50, 50);

	/* reserve less elements than inserting */
	reserve_and_insert(argv[1], argv[2], 10, 100);

	/* insert, reopen database and reserve less elements */
	insert_and_reserve(argv[1], argv[2], 100, 10);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
