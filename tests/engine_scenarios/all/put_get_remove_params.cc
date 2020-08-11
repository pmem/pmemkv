// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void LargeAscendingTest(const size_t iterations, pmem::kv::db &kv)
{
	for (size_t i = 1; i <= iterations; i++) {
		std::string istr = std::to_string(i);
		ASSERT_STATUS(kv.put(istr, (istr + "!")), status::OK);
		std::string value;
		ASSERT_STATUS(kv.get(istr, &value), status::OK);
		UT_ASSERT(value == (istr + "!"));
	}
	for (size_t i = 1; i <= iterations; i++) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_STATUS(kv.get(istr, &value), status::OK);
		UT_ASSERT(value == (istr + "!"));
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == iterations);
}

static void LargeDescendingTest(const size_t iterations, pmem::kv::db &kv)
{
	for (size_t i = iterations; i >= 1; i--) {
		std::string istr = std::to_string(i);
		ASSERT_STATUS(kv.put(istr, ("ABC" + istr)), status::OK);
		std::string value;
		ASSERT_STATUS(kv.get(istr, &value), status::OK);
		UT_ASSERT(value == ("ABC" + istr));
	}
	for (size_t i = iterations; i >= 1; i--) {
		std::string istr = std::to_string(i);
		std::string value;
		ASSERT_STATUS(kv.get(istr, &value), status::OK);
		UT_ASSERT(value == ("ABC" + istr));
	}
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == iterations);
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 4)
		UT_FATAL("usage: %s engine json_config iterations", argv[0]);

	auto iterations = std::stoull(argv[3]);

	run_engine_tests(argv[1], argv[2],
			 {
				 std::bind(LargeAscendingTest, iterations, _1),
				 std::bind(LargeDescendingTest, iterations, _1),
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
