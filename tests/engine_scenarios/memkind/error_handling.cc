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

static void FailsToOpenInstanceWithInvalidPath(std::string engine, std::string non_existent_path)
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
