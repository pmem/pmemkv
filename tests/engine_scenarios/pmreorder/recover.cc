// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * recover.cc -- recover pmreorder test
 */

#include "unittest.hpp"

static void check_consistency(std::string engine, std::string config)
{
	auto kv = INITIALIZE_KV(engine, CONFIG_FROM_JSON(config));

	size_t cnt = std::numeric_limits<size_t>::max();

	ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);
	UT_ASSERTeq(cnt, 0);

	for (size_t i = 0; i < 100; ++i)
		ASSERT_STATUS(kv.put(entry_from_number(i, "", "key"),
				     entry_from_number(i, "", "val")),
			      pmem::kv::status::OK);

	ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);
	UT_ASSERTeq(cnt, 100);

	for (size_t i = 0; i < 100; ++i) {
		std::string val;
		ASSERT_STATUS(kv.get(entry_from_number(i, "", "key"), &val),
			      pmem::kv::status::OK);
		UT_ASSERT(val == entry_from_number(i, "", "val"));

		ASSERT_STATUS(kv.remove(entry_from_number(i, "", "key")),
			      pmem::kv::status::OK);
	}

	ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);
	UT_ASSERTeq(cnt, 0);

	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config <open|create>", argv[0]);

	std::string mode = argv[3];
	if (mode != "open" && mode != "create")
		UT_FATAL("usage: %s engine json_config <open|create>", argv[0]);

	if (mode == "open") {
		check_consistency(argv[1], argv[2]);
	} else if (mode == "create") {
		INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
