// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void SimpleMultithreadedTest(pmem::kv::db &kv)
{

	size_t threads_number = 8;
	size_t thread_items = 50;
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
	// std::size_t cnt = std::numeric_limits<std::size_t>::max();
	// UT_ASSERT(kv.count_all(cnt) == status::OK);
	// UT_ASSERT(cnt == threads_number * thread_items);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 SimpleMultithreadedTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
