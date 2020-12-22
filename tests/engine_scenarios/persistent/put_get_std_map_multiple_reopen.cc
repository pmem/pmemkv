// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../put_get_std_map.hpp"

const int N_ITERS = 50;

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 6)
		UT_FATAL("usage: %s engine json_config n_inserts key_length value_length",
			 argv[0]);

	auto n_inserts = std::stoull(argv[3]);
	auto key_length = std::stoull(argv[4]);
	auto value_length = std::stoull(argv[5]);

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	auto proto = PutToMapTest(n_inserts, key_length, value_length, kv);

	for (int i = 0; i < N_ITERS; i++) {
		kv.close();
		kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));
		VerifyKv(proto, kv);
	}

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
