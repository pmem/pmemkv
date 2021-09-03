// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/*
 * insert.cc -- insert pmreorder test
 */

#include "unittest.hpp"

static constexpr size_t len_elements = 10;

static void check_exist(pmem::kv::db &kv, const std::string &element,
			pmem::kv::status exists)
{
	std::string value;
	UT_ASSERT(kv.get(element, &value) == exists);

	if (exists == pmem::kv::status::OK) {
		UT_ASSERT(element == value);
	}
}

static void test_init(pmem::kv::db &kv)
{
	for (size_t i = 0; i < len_elements; i++) {
		std::string element = entry_from_number(i);
		ASSERT_STATUS(kv.put(element, element), pmem::kv::status::OK);
		check_exist(kv, element, pmem::kv::status::OK);
	}
}

static void test_insert(pmem::kv::db &kv)
{
	ASSERT_SIZE(kv, len_elements);

	std::string element = entry_from_number(len_elements);
	ASSERT_STATUS(kv.put(element, element), pmem::kv::status::OK);
	check_exist(kv, element, pmem::kv::status::OK);
}

static void check_consistency(pmem::kv::db &kv)
{
	std::size_t size;
	ASSERT_STATUS(kv.count_all(size), pmem::kv::status::OK);
	std::size_t count = 0;

	for (size_t i = 0; i <= len_elements; i++) {
		std::string element = entry_from_number(i);
		if (kv.exists(element) == pmem::kv::status::OK) {
			++count;
			check_exist(kv, element, pmem::kv::status::OK);
		} else {
			check_exist(kv, element, pmem::kv::status::NOT_FOUND);
		}
	}

	UT_ASSERTeq(count, size);
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
		test_insert(kv);
	}

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
