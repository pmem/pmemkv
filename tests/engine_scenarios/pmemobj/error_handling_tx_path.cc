// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "error_handling_tx.hpp"

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 4)
		UT_FATAL("usage: %s engine json_config obj_path", argv[0]);

	auto pmemobj_pool_path = std::string(argv[3]);
	pmem::obj::pool_base pmemobj_pool;

	try {
		pmemobj_pool = pmem::obj::pool_base::open(pmemobj_pool_path, "pmemkv");
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	TransactionTest(pmemobj_pool, kv);

	pmemobj_pool.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
