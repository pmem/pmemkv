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

static void OOM(pmem::kv::db &kv)
{
	size_t cnt = 0;
	while (1) {
		auto s = kv.put(std::to_string(cnt), std::string(cnt + 1, 'a'));
		if (s == pmem::kv::status::OUT_OF_MEMORY)
			break;

		UT_ASSERTeq(s, pmem::kv::status::OK);

		cnt++;
	}

	/* At least one iteration */
	UT_ASSERT(cnt > 0);

	/* Start freeing elements from the smallest one */
	for (size_t i = 0; i < cnt; i++) {
		auto s = kv.remove(std::to_string(i));
		UT_ASSERTeq(s, pmem::kv::status::OK);
	}

	size_t count = std::numeric_limits<size_t>::max();
	auto s = kv.count_all(count);
	UT_ASSERTeq(s, pmem::kv::status::OK);
	UT_ASSERTeq(count, 0);
}

static void test(int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2], {OOM});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
