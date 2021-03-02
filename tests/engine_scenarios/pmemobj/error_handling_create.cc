// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj/pool_base.h>

static void FailsToCreateInstanceWithNonExistentPath(std::string non_existent_path,
						     std::string engine, bool error_flag)
{
	pmem::kv::config config;
	auto s = config.put_path(non_existent_path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Non-existent path supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToOpenInstanceWithNonExistentPath(std::string non_existent_path,
						   std::string engine, bool error_flag)
{
	pmem::kv::config config;
	auto s = config.put_path(non_existent_path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Non-existent path supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToCreateInstanceWithHugeSize(std::string path, std::string engine,
					      bool error_flag)
{
	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(std::numeric_limits<uint64_t>::max());
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Too big pool size supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToCreateInstanceWithTinySize(std::string path, std::string engine,
					      bool error_flag)
{
	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(PMEMOBJ_MIN_POOL - 1);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Too small pool size supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToCreateInstanceWithNoSize(std::string path, std::string engine,
					    bool error_flag)
{
	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* No size supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToCreateInstanceWithPathAndOid(std::string path, std::string engine,
						bool error_flag)
{
	PMEMoid oid;

	pmem::kv::config config;
	auto s = config.put_path(path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_oid(&oid);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Both path and oid supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToCreateInstanceWithNoPathOrOid(std::string path, std::string engine,
						 bool error_flag)
{
	pmem::kv::config config;
	auto s = config.put_create_or_error_if_exists(error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_create_if_missing(!error_flag);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* No path and no oid supplied */
	ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
}

static void FailsToCreateInstanceWithCornerCasePaths(std::string engine, bool error_flag)
{
	std::vector<std::string> paths = {"/", "", "//",
					  ",./;'[]-=<>?:\"{}|_+!@#$%^&*()`~"};

	for (auto &path : paths) {
		pmem::kv::config config;
		auto s = config.put_create_or_error_if_exists(true);
		ASSERT_STATUS(s, pmem::kv::status::OK);
		s = config.put_create_if_missing(true);
		ASSERT_STATUS(s, pmem::kv::status::OK);
		s = config.put_size(5 * PMEMOBJ_MIN_POOL);
		ASSERT_STATUS(s, pmem::kv::status::OK);
		s = config.put_path(path);
		ASSERT_STATUS(s, pmem::kv::status::OK);

		pmem::kv::db kv;
		s = kv.open(engine, std::move(config));

		/* Invalid path supplied */
		ASSERT_STATUS(s, pmem::kv::status::INVALID_ARGUMENT);
	}
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine path non_existent_path", argv[0]);

	auto engine = argv[1];
	auto path = argv[2];
	auto non_existent_path = argv[3];

	for (auto flag : {true, false}) {
		FailsToCreateInstanceWithNonExistentPath(non_existent_path, engine, flag);
		FailsToOpenInstanceWithNonExistentPath(non_existent_path, engine, flag);
		FailsToCreateInstanceWithHugeSize(path, engine, flag);
		FailsToCreateInstanceWithTinySize(path, engine, flag);
		FailsToCreateInstanceWithNoSize(path, engine, flag);
		FailsToCreateInstanceWithPathAndOid(path, engine, flag);
		FailsToCreateInstanceWithNoPathOrOid(path, engine, flag);
		FailsToCreateInstanceWithCornerCasePaths(engine, flag);
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
