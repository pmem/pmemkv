// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static std::vector<std::string> initial_elements = {"0", "1"};
static std::vector<std::string> elements = {"A", "B", "C"};

static void test_init(pmem::kv::db &kv)
{
	for (auto &e : initial_elements)
		ASSERT_STATUS(kv.put(e, e), pmem::kv::status::OK);
}

static void test_put_commit(pmem::kv::db &kv)
{
	auto tx = kv.tx_begin();
	for (auto &e : elements)
		ASSERT_STATUS(tx.put(e, e), pmem::kv::status::OK);

	ASSERT_STATUS(tx.commit(), pmem::kv::status::OK);
}

static void check_consistency(pmem::kv::db &kv)
{
	size_t size;
	ASSERT_STATUS(kv.count_all(size), pmem::kv::status::OK);

	if (size > initial_elements.size()) {
		UT_ASSERT(size == initial_elements.size() + elements.size());

		for (auto &e : initial_elements)
			ASSERT_STATUS(kv.exists(e), pmem::kv::status::OK);

		for (auto &e : elements)
			ASSERT_STATUS(kv.exists(e), pmem::kv::status::OK);
	} else {
		UT_ASSERT(size == initial_elements.size());
	}
}

static void test(int argc, char *argv[])
{
	std::cout << "ARGC: " << argc << std::endl;
	for (int i = 0; i < argc; ++i) {
		std::cout << "ARGV " << i << " : " << argv[i] << std::endl;
	}
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config <create|open|insert>", argv[0]);

	std::string mode = argv[3];
	if (mode != "create" && mode != "open" && mode != "insert")
		UT_FATAL("usage: %s engine json_config <create|open|insert>", argv[0]);

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	if (mode == "create") {
		test_init(kv);
	} else if (mode == "open") {
		check_consistency(kv);
	} else if (mode == "insert") {
		test_put_commit(kv);
	}

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
