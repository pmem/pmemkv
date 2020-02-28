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

#include "mock_tx_alloc.h"
#include "unittest.hpp"

const std::string LONGSTR =
	"123456789A123456789A123456789A123456789A123456789A123456789A123456789A";

const int init_iterations = 50000;

void validate(pmem::kv::db &kv)
{
	for (size_t i = 0; i < init_iterations; i++) {
		std::string istr = std::to_string(i);
		auto s = kv.get(istr, [&](pmem::kv::string_view val) {
			UT_ASSERT(val.compare(istr + "!") == 0);
		});
		UT_ASSERTeq(s, pmem::kv::status::OK);
	}
}

void populate(pmem::kv::db &kv)
{
	for (size_t i = 0; i < init_iterations; i++) {
		std::string istr = std::to_string(i);
		UT_ASSERTeq(kv.put(istr, (istr + "!")), pmem::kv::status::OK);
	}
}

void LongStringTest(pmem::kv::db &kv)
{
	populate(kv);

	UT_ASSERT(kv.remove("100") == pmem::kv::status::OK);
	tx_alloc_should_fail = true;
	UT_ASSERT(kv.put("100", LONGSTR) == pmem::kv::status::OUT_OF_MEMORY);
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.put("100", "100!") == pmem::kv::status::OK);

	validate(kv);
}

void ShortKeyTest(pmem::kv::db &kv)
{
	populate(kv);

	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		UT_ASSERT(kv.put("123456", LONGSTR) == pmem::kv::status::OUT_OF_MEMORY);
	}
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.remove("4567") == pmem::kv::status::OK);
	UT_ASSERT(kv.put("4567", "4567!") == pmem::kv::status::OK);

	validate(kv);
}

void LongKeyTest(pmem::kv::db &kv)
{
	populate(kv);

	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		UT_ASSERT(kv.put(LONGSTR, "1") == pmem::kv::status::OUT_OF_MEMORY);
		UT_ASSERT(kv.put(LONGSTR, LONGSTR) == pmem::kv::status::OUT_OF_MEMORY);
	}
	tx_alloc_should_fail = false;
	UT_ASSERT(kv.remove("34567") == pmem::kv::status::OK);
	UT_ASSERT(kv.put("34567", "34567!") == pmem::kv::status::OK);

	validate(kv);
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 4)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 LongStringTest,
				 ShortKeyTest,
				 LongKeyTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
