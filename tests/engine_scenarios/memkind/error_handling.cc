// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static void FailsToOpenInstanceWithInvalidPath(std::string engine,
					       std::string non_existent_path)
{
	pmem::kv::config cfg;
	auto s = cfg.put_path(non_existent_path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = cfg.put_size(83886080);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	/* Not-existent path supplied */
	// XXX - should be WRONG_PATH
	ASSERT_STATUS(s, pmem::kv::status::UNKNOWN_ERROR);
}

static void NoSizeInConfig(std::string engine)
{
	pmem::kv::config cfg;
	auto s = cfg.put_path("some_path");
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void NoPathInConfig(std::string engine)
{
	pmem::kv::config cfg;
	auto s = cfg.put_size(83886080);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void test(int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: %s engine non_existent_path", argv[0]);

	FailsToOpenInstanceWithInvalidPath(argv[1], argv[2]);
	NoSizeInConfig(argv[1]);
	NoPathInConfig(argv[1]);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
