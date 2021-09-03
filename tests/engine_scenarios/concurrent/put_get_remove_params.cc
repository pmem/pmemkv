// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void SimpleMultithreadedTest(const size_t threads_number,
				    const size_t thread_items, pmem::kv::db &kv)
{
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string key = entry_from_number(i);
			std::string val = entry_from_number(i, "", "!");
			ASSERT_STATUS(kv.put(key, val), status::OK);
			std::string value;
			ASSERT_STATUS(kv.get(key, &value), status::OK);
			UT_ASSERT(value == val);
		}
		for (auto i = begin; i < end; i++) {
			std::string key = entry_from_number(i);
			std::string val = entry_from_number(i, "", "!");
			std::string value;
			ASSERT_STATUS(kv.get(key, &value), status::OK);
			UT_ASSERT(value == val);
		}
	});
	ASSERT_SIZE(kv, threads_number * thread_items);
}

static void MultithreadedTestRemoveDataAside(const size_t threads_number,
					     const size_t thread_items, pmem::kv::db &kv)
{
	size_t initial_items = 128;
	/* put initial data, which won't be touched */
	for (size_t i = 0; i < initial_items; i++) {
		std::string key = entry_from_number(i, "in_");
		std::string val = entry_from_number(i, "in_", "!");
		ASSERT_STATUS(kv.put(key, val), status::OK);
	}

	/* test adding and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string key = entry_from_number(i);
			std::string val = entry_from_number(i, "", "!");
			ASSERT_STATUS(kv.put(key, val), status::OK);
		}
		for (auto i = begin; i < end; i++) {
			std::string key = entry_from_number(i);
			std::string val = entry_from_number(i, "", "!");
			std::string value;
			ASSERT_STATUS(kv.get(key, &value), status::OK);
			UT_ASSERT(value == val);
			ASSERT_STATUS(kv.remove(key), status::OK);
		}
	});
	ASSERT_SIZE(kv, initial_items);

	/* get initial data and confirm it's untouched */
	for (size_t i = 0; i < initial_items; i++) {
		std::string key = entry_from_number(i, "in_");
		std::string val = entry_from_number(i, "in_", "!");
		std::string value;
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == val);
	}
}

static void MultithreadedPutRemove(const size_t threads_number, const size_t thread_items,
				   pmem::kv::db &kv)
{
	size_t initial_items = threads_number * thread_items;
	for (size_t i = 0; i < initial_items; i++) {
		std::string key = entry_from_number(i);
		std::string value = entry_from_number(i, "", "!");
		ASSERT_STATUS(kv.put(key, value), status::OK);
	}

	parallel_exec(threads_number, [&](size_t thread_id) {
		if (thread_id < threads_number / 2) {
			for (size_t i = 0; i < initial_items; i++) {
				std::string key = entry_from_number(i);
				std::string value = entry_from_number(i, "", "!");
				ASSERT_STATUS(kv.put(key, value), status::OK);
			}
		} else {
			for (size_t i = 0; i < initial_items; i++) {
				std::string key = entry_from_number(i);
				auto s = kv.remove(key);
				UT_ASSERT(s == status::OK || s == status::NOT_FOUND);
			}
		}
	});

	for (size_t i = 0; i < initial_items; i++) {
		std::string key = entry_from_number(i);
		std::string value = entry_from_number(i, "", "!");
		std::string val;
		auto s = kv.get(key, &val);
		UT_ASSERT((s == status::OK && val == value) || s == status::NOT_FOUND);
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
				 std::bind(MultithreadedPutRemove, threads_number,
					   thread_items, _1),
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
