// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

/*
 * Tests for config flags.
 * Setting create_if_missing to control kv.open() or unsetting both flags
 * should not fail in any engine.
 * If engine supports these flags the scenarios below, should just open the pool.
 * Engines with no support for these flags should just not read them.
 */

static void OpenWithCreateIfMissing(std::string path, std::string engine, size_t size,
				    bool flag_value)
{
	/**
	 * TEST: create_if_missing should work fine with either setting, on existing pool.
	 */

	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(size);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(flag_value);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));
	ASSERT_STATUS(s, pmem::kv::status::OK);
}

static void OpenWithBothFlagsFalse(std::string path, std::string engine, size_t size)
{
	/**
	 * TEST: both flags set to false, it should just open pool.
	 */

	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(size);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(false);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(false);
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

	OpenWithBothFlagsFalse(path, engine, size);

	for (auto flag : {true, false}) {
		OpenWithCreateIfMissing(path, engine, size, flag);
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
