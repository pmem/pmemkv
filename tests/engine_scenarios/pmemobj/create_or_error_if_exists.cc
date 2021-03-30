// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

/*
 * Tests for config flag create_or_error_if_exists with existing pool.
 */

static void FailsToOpenExisting(std::string path, std::string engine, std::size_t size)
{
	/**
	 * TEST: create_or_error_if_exists set to **true** should not work with existing
	 * pool.
	 */

	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(size);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(true);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* It should err with "Failed creating pool - already exists" */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void OpenExisting(std::string path, std::string engine, std::size_t size)
{
	/**
	 * TEST: create_or_error_if_exists set to **false** should work with existing
	 * pool.
	 */

	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(size);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(false);
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

	FailsToOpenExisting(path, engine, size);
	OpenExisting(path, engine, size);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
