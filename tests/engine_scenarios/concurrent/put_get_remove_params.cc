// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void SimpleMultithreadedTest(const size_t threads_number,
				    const size_t thread_items, pmem::kv::db &kv)
{
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string key = generate_key(i);
			std::string val = generate_key(i, "", "!");
			ASSERT_STATUS(kv.put(key, val), status::OK);
			std::string value;
			ASSERT_STATUS(kv.get(key, &value), status::OK);
			UT_ASSERT(value == val);
		}
		for (auto i = begin; i < end; i++) {
			std::string key = generate_key(i);
			std::string val = generate_key(i, "", "!");
			std::string value;
			ASSERT_STATUS(kv.get(key, &value), status::OK);
			UT_ASSERT(value == val);
		}
	});
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == threads_number * thread_items);
}

static void MultithreadedTestRemoveDataAside(const size_t threads_number,
					     const size_t thread_items, pmem::kv::db &kv)
{
	size_t initial_items = 128;
	/* put initial data, which won't be touched */
	for (size_t i = 0; i < initial_items; i++) {
		std::string key = generate_key(i, "in_");
		std::string val = generate_key(i, "in_", "!");
		ASSERT_STATUS(kv.put(key, val), status::OK);
	}

	/* test adding and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string key = generate_key(i);
			std::string val = generate_key(i, "", "!");
			ASSERT_STATUS(kv.put(key, val), status::OK);
		}
		for (auto i = begin; i < end; i++) {
			std::string key = generate_key(i);
			std::string val = generate_key(i, "", "!");
			std::string value;
			ASSERT_STATUS(kv.get(key, &value), status::OK);
			UT_ASSERT(value == val);
			ASSERT_STATUS(kv.remove(key), status::OK);
		}
	});
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == initial_items);

	/* get initial data and confirm it's untouched */
	for (size_t i = 0; i < initial_items; i++) {
		std::string key = generate_key(i, "in_");
		std::string val = generate_key(i, "in_", "!");
		std::string value;
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == val);
	}
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 5)
		UT_FATAL("usage: %s engine json_config threads items", argv[0]);

	size_t threads_number = std::stoull(argv[3]);
	size_t thread_items = std::stoull(argv[4]);
	run_engine_tests(argv[1], argv[2],
			 {
				 std::bind(SimpleMultithreadedTest, threads_number,
					   thread_items, _1),
				 std::bind(MultithreadedTestRemoveDataAside,
					   threads_number, thread_items, _1),
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
