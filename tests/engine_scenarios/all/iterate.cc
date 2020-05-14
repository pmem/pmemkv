// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"

#include <algorithm>
#include <vector>

/**
 * Tests get_all and count_all methods for unsorted engines.
 * Since we cannot assume any order, custom sort is applied to results before comparing.
 */

/* XXX: are we missing check for unsorted engines, if get/count_* methods are not
 * supported...? */
/* XXX: this test should be extended with more data and some removal */

using namespace pmem::kv;

using test_kv = std::pair<std::string, std::string>;
using test_kv_list = std::vector<test_kv>;

static test_kv_list sort(test_kv_list list)
{
	std::sort(list.begin(), list.end(), [](const test_kv &lhs, const test_kv &rhs) {
		return lhs.first < rhs.first;
	});

	return list;
}

static void GetAllTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_all should return all elements in db and count_all should count them
	 * properly
	 */
	UT_ASSERTeq(kv.put("1", "one"), status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 1);
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.put("2", "two") == status::OK);
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 2);
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.put("记!", "RR") == status::OK);
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 3);

	test_kv_list result;
	/* get_all using string_view */
	auto s = kv.get_all([&](string_view k, string_view v) {
		result.emplace_back(std::string(k.data(), k.size()),
				    std::string(v.data(), v.size()));
		return 0;
	});
	UT_ASSERTeq(s, status::OK);

	auto expected = test_kv_list{{"1", "one"}, {"2", "two"}, {"记!", "RR"}};
	UT_ASSERT((sort(result) == sort(expected)));
	result = {};

	/* get_all with C-like API */
	s = kv.get_all(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((test_kv_list *)arg);
			c->emplace_back(std::string(k, kb), std::string(v, vb));
			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);

	expected = test_kv_list{{"1", "one"}, {"2", "two"}, {"记!", "RR"}};
	UT_ASSERT((sort(result) == sort(expected)));
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2], {GetAllTest});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
