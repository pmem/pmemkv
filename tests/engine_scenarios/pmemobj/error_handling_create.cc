/*
 * Copyright 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "unittest.hpp"

#include <libpmemobj/pool_base.h>

static void FailsToCreateInstanceWithNonExistentPath(std::string non_existent_path,
						     std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_string("path", non_existent_path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("force_create", 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("size", 5 * PMEMOBJ_MIN_POOL);
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
	auto s = config.put_string("path", path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("force_create", 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("size", 9223372036854775807);
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
	auto s = config.put_string("path", path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("force_create", 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("size", PMEMOBJ_MIN_POOL - 1);
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
	auto s = config.put_string("path", path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("force_create", 1);
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
	auto s = config.put_string("path", path);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_object("oid", &oid, nullptr);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("force_create", 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("size", 5 * PMEMOBJ_MIN_POOL);
	UT_ASSERTeq(pmem::kv::status::OK, s);

	pmem::kv::db kv;
	s = kv.open(engine, std::move(config));

	/* Both path and oid supplied */
	UT_ASSERTeq(pmem::kv::status::INVALID_ARGUMENT, s);
}

static void FailsToCreateInstanceWithNoPathAndOid(std::string path, std::string engine)
{
	pmem::kv::config config;
	auto s = config.put_uint64("force_create", 1);
	UT_ASSERTeq(pmem::kv::status::OK, s);
	s = config.put_uint64("size", 5 * PMEMOBJ_MIN_POOL);
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
