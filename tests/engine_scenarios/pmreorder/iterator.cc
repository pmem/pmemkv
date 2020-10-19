// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * iterator.cc -- iterator pmreorder test
 */

#include "unittest.hpp"

static constexpr int len_elements = 20;

static void check_exist(pmem::kv::db &kv, const std::string &key,
			const std::string &value)
{
	auto it = kv.new_read_iterator();
	UT_ASSERTeq(it.seek(key), pmem::kv::status::OK);

	auto res = it.read_range(0, std::numeric_limits<size_t>::max());
	ASSERT_STATUS(res.second, pmem::kv::status::OK);
	UT_ASSERTeq(value.compare(res.first.begin()), 0);
}

static void test_init(pmem::kv::db &kv)
{
	for (int i = 0; i < len_elements; i++) {
		auto key = std::to_string(i);
		auto val = std::string(len_elements, static_cast<char>(i + 10));
		ASSERT_STATUS(kv.put(key, val), pmem::kv::status::OK);
		check_exist(kv, key, val);
	}
}

/* write first element's value to 'x' */
static void test_write(pmem::kv::db &kv)
{
	auto it = kv.new_write_iterator();

	std::size_t size;
	ASSERT_STATUS(kv.count_all(size), pmem::kv::status::OK);
	UT_ASSERT(size == len_elements);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	auto res = it.write_range(0, std::numeric_limits<size_t>::max());
	ASSERT_STATUS(res.second, pmem::kv::status::OK);
	for (auto &c : res.first)
		c = 'x';
	it.commit();

	check_exist(kv, "0", std::string(len_elements, 'x'));
}

static void check_consistency(pmem::kv::db &kv)
{
	auto it = kv.new_read_iterator();

	std::size_t size;
	ASSERT_STATUS(kv.count_all(size), pmem::kv::status::OK);
	std::size_t count = 0;

	for (int i = 1; i < len_elements; i++) {
		std::string key = std::to_string(i);
		std::string val = std::string(len_elements, static_cast<char>(i + 10));
		ASSERT_STATUS(it.seek(key), pmem::kv::status::OK);
		++count;
		check_exist(kv, key, val);
	}

	/* check first element's value */
	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);
	auto res = it.read_range(0, std::numeric_limits<size_t>::max());
	ASSERT_STATUS(res.second, pmem::kv::status::OK);
	std::string value = res.first.begin();
	UT_ASSERT(value.compare(std::string(len_elements, (char)10)) == 0 ||
		  value.compare(std::string(len_elements, 'x')) == 0);

	++count;
	UT_ASSERTeq(count, size);
}

static void test(int argc, char *argv[])
{
	std::cout << "ARGC: " << argc << std::endl;
	for (int i = 0; i < argc; ++i) {
		std::cout << "ARGV " << i << " : " << argv[i] << std::endl;
	}
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config <create|open|write>", argv[0]);

	std::string mode = argv[3];
	if (mode != "create" && mode != "open" && mode != "write")
		UT_FATAL("usage: %s engine json_config <create|open|write>", argv[0]);

	auto kv = INITIALIZE_KV(argv[1], CONFIG_FROM_JSON(argv[2]));

	if (mode == "create") {
		test_init(kv);
	} else if (mode == "open") {
		check_consistency(kv);
	} else if (mode == "write") {
		test_write(kv);
	}

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
