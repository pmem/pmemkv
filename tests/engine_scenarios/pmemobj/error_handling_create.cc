// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj/pool_base.h>

static void FailsToCreateInstanceWithNonExistentPath(std::string non_existent_path,
						     std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_path(non_existent_path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_force_create(true);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Not-existent path supplied */
	// XXX - should be WRONG_PATH
	UT_ASSERTeq(pmem::kv::status::UNKNOWN_ERROR, s);
}

static void FailsToCreateInstanceWithHugeSize(std::string path, std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_path(path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_force_create(true);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_size(9223372036854775807);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Too big pool size supplied */
	// XXX - should be WRONG_SIZE
	UT_ASSERTeq(pmem::kv::status::UNKNOWN_ERROR, s);
}

static void FailsToCreateInstanceWithTinySize(std::string path, std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_path(path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_force_create(true);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_size(PMEMOBJ_MIN_POOL - 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Too small pool size supplied */
	// XXX - should be WRONG_SIZE
	UT_ASSERTeq(pmem::kv::status::UNKNOWN_ERROR, s);
}

static void FailsToCreateInstanceWithNoSize(std::string path, std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_path(path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_force_create(true);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* No size supplied */
	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);
}

static void FailsToCreateInstanceWithPathAndOid(std::string path, std::string engine)
{
	PMEMoid oid;

	pmem::kv::config config;
	auto s = config.put_path(path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_oid(&oid);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_force_create(true);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Both path and oid supplied */
	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);
}

static void FailsToCreateInstanceWithNoPathAndOid(std::string path, std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_force_create(true);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_size(5 * PMEMOBJ_MIN_POOL);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* No path and no oid supplied */
	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine path non_existent_path", argv[0]);

	auto engine = argv[1];
	auto path = argv[2];
	auto non_existent_path = argv[3];

	FailsToCreateInstanceWithNonExistentPath(non_existent_path, engine);
	FailsToCreateInstanceWithHugeSize(path, engine);
	FailsToCreateInstanceWithTinySize(path, engine);
	FailsToCreateInstanceWithNoSize(path, engine);
	FailsToCreateInstanceWithPathAndOid(path, engine);
	FailsToCreateInstanceWithNoPathAndOid(path, engine);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
