// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void SimpleMultithreadedTest(const size_t threads_number,
				    const size_t thread_items, pmem::kv::db &kv)
{
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, (istr + "!")) == status::OK);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK &&
				  value == (istr + "!"));
		}
		for (auto i = begin; i < end; i++) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK &&
				  value == (istr + "!"));
		}
	});
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == threads_number * thread_items);
}

static void MultithreadedTestRemoveDataAside(const size_t threads_number,
					     const size_t thread_items, pmem::kv::db &kv)
{
	size_t initial_items = 128;
	/* put initial data, which won't be touched */
	for (size_t i = 0; i < initial_items; i++) {
		std::string istr = "init_" + std::to_string(i);
		UT_ASSERT(kv.put(istr, (istr + "!")) == status::OK);
	}

	/* test adding and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, (istr + "!")) == status::OK);
		}
		for (auto i = begin; i < end; i++) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK &&
				  value == (istr + "!"));
			UT_ASSERT(kv.remove(istr) == status::OK);
		}
	});
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == initial_items);

	/* get initial data and confirm it's untouched */
	for (size_t i = 0; i < initial_items; i++) {
		std::string istr = "init_" + std::to_string(i);
		std::string value;
		UT_ASSERT(kv.get(istr, &value) == status::OK && value == (istr + "!"));
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
