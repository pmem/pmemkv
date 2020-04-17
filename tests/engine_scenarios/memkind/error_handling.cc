// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

static void FailsToOpenInstanceWithInvalidPath(std::string engine,
					       std::string non_existent_path)
{
	pmem::kv::config cfg;
	auto s = cfg.put_string("path", non_existent_path);
	UT_ASSERTeq(s, pmem::kv::status::OK);
	s = cfg.put_uint64("size", 83886080);
	UT_ASSERTeq(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	/* Not-existent path supplied */
	// XXX - should be WRONG_PATH
	UT_ASSERTeq(pmem::kv::status::UNKNOWN_ERROR, s);
}

static void NoSizeInConfig(std::string engine)
{
	pmem::kv::config cfg;
	auto s = cfg.put_string("path", "some_path");
	UT_ASSERTeq(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);
}

static void NoPathInConfig(std::string engine)
{
	pmem::kv::config cfg;
	auto s = cfg.put_uint64("size", 83886080);
	UT_ASSERTeq(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(cfg));

	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);
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
