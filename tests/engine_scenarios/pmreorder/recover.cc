// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * recover.cc -- recover pmreorder test
 */

#include "unittest.hpp"

static void test_create(std::string engine, std::string config)
{
	auto kv = INITIALIZE_KV(engine, CONFIG_FROM_JSON(config));

	kv.close();
}

static void check_consistency(std::string engine, std::string config)
{
	auto kv = INITIALIZE_KV(engine, CONFIG_FROM_JSON(config));

	size_t cnt = std::numeric_limits<size_t>::max();

	ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);
	UT_ASSERT(cnt == 0);

	for (size_t i = 0; i < 1000; ++i)
		ASSERT_STATUS(kv.put(entry_from_number(i, "", "key"),
				     entry_from_number(i, "", "val")),
			      pmem::kv::status::OK);

	ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);
	UT_ASSERT(cnt == 1000);

	for (size_t i = 0; i < 1000; ++i) {
		std::string val;

		ASSERT_STATUS(kv.get(entry_from_number(i, "", "key"), &val),
			      pmem::kv::status::OK);
		UT_ASSERT(val == entry_from_number(i, "", "val"));

		ASSERT_STATUS(kv.remove(entry_from_number(i, "", "key")),
			      pmem::kv::status::OK);
		ASSERT_STATUS(kv.get(entry_from_number(i, "", "key"), &val),
			      pmem::kv::status::NOT_FOUND);
	}

	ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);
	UT_ASSERT(cnt == 0);

	kv.close();
}

static void test(int argc, char *argv[])
{
	std::cout << "ARGC: " << argc << std::endl;
	for (int i = 0; i < argc; ++i) {
		std::cout << "ARGV " << i << " : " << argv[i] << std::endl;
	}
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config <open|create>", argv[0]);

	std::string mode = argv[3];
	if (mode != "open" && mode != "create")
		UT_FATAL("usage: %s engine json_config <open|create>", argv[0]);

	if (mode == "open") {
		check_consistency(argv[1], argv[2]);
	} else if (mode == "create") {
		test_create(argv[1], argv[2]);
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
