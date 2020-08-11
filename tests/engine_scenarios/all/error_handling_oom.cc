// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static void OOM(pmem::kv::db &kv)
{
	size_t cnt = 0;
	while (1) {
		auto s = kv.put(std::to_string(cnt), std::string(cnt + 1, 'a'));
		if (s == pmem::kv::status::OUT_OF_MEMORY)
			break;

		ASSERT_STATUS(s, pmem::kv::status::OK);

		cnt++;
	}

	/* At least one iteration */
	UT_ASSERT(cnt > 0);

	/* Start freeing elements from the smallest one */
	for (size_t i = 0; i < cnt; i++) {
		auto s = kv.remove(std::to_string(i));
		ASSERT_STATUS(s, pmem::kv::status::OK);
	}

	size_t count = std::numeric_limits<size_t>::max();
	auto s = kv.count_all(count);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	UT_ASSERTeq(count, 0);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2], {OOM});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
