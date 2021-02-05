// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void read_iterator_not_supported(pmem::kv::db &kv)
{
	auto res = kv.new_read_iterator();
	UT_ASSERT(!res.is_ok());

	auto s = res.get_status();
	ASSERT_STATUS(s, status::NOT_SUPPORTED);
}

static void write_iterator_not_supported(pmem::kv::db &kv)
{
	auto res = kv.new_write_iterator();
	UT_ASSERT(!res.is_ok());

	auto s = res.get_status();
	ASSERT_STATUS(s, status::NOT_SUPPORTED);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 write_iterator_not_supported,
				 read_iterator_not_supported,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
