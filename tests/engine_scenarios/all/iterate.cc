/*
 * Copyright 2017-2020, Intel Corporation
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

using namespace pmem::kv;

static void UsesGetAllTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("1", "2") == status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.put("RR", "记!") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 2);

	std::string result;
	kv.get_all(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERT(result == "<1>,<2>|<RR>,<记!>|");
}

static void UsesGetAllTest2(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("1", "one") == status::OK);
	UT_ASSERT(kv.put("2", "two") == status::OK);
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	std::string x;
	kv.get_all([&](string_view k, string_view v) {
		x.append("<")
			.append(k.data(), k.size())
			.append(">,<")
			.append(v.data(), v.size())
			.append(">|");
		return 0;
	});
	UT_ASSERT(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

	x = "";
	kv.get_all([&](string_view k, string_view v) {
		x.append("<")
			.append(std::string(k.data(), k.size()))
			.append(">,<")
			.append(std::string(v.data(), v.size()))
			.append(">|");
		return 0;
	});
	UT_ASSERT(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");

	x = "";
	kv.get_all(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<")
				.append(std::string(k, kb))
				.append(">,<")
				.append(std::string(v, vb))
				.append(">|");
			return 0;
		},
		&x);
	UT_ASSERT(x == "<1>,<one>|<2>,<two>|<记!>,<RR>|");
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2], {UsesGetAllTest, UsesGetAllTest2});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
