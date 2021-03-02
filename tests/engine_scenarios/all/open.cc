// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

static void OpenWithBothFlags(std::string path, std::string engine, size_t size,
			      bool flags_value)
{
	/**
	 * TEST: Setting both config flags to control kv open should not fail in
	 * any engine. If engine supports them, one of them should be prioritzed,
	 * and the second one should be ignored. Engines with no support for these
	 * flags should just not read them.
	 */

	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(size);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_error_if_exists(flags_value);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(flags_value);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));
	ASSERT_STATUS(s, pmem::kv::status::OK);
}

static void test(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s engine path size", argv[0]);

	auto engine = argv[1];
	auto path = argv[2];
	size_t size = std::stoul(argv[3]);

	OpenWithBothFlags(path, engine, size, true);
	// OpenWithBothFlags(path, engine, size, false);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
