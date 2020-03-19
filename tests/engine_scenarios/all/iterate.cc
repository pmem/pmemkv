// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

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
