// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * Helper functions to test iterators.
 */

#include "../common/unittest.hpp"

template <bool IsConst>
using iterator = typename std::conditional<IsConst, pmem::kv::db::read_iterator,
					   pmem::kv::db::write_iterator>::type;

using pair = std::pair<std::string, std::string>;

using key_result = std::pair<pmem::kv::string_view, pmem::kv::status>;

static std::vector<pair> keys{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},	 {"rrr", "4"},
			      {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};

void insert_keys(pmem::kv::db &kv)
{
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(kv.put(p.first, p.second), pmem::kv::status::OK);
	});
}

template <bool IsConst>
void verify_key(iterator<IsConst> &it, pmem::kv::string_view expected)
{
	auto result = it.key();
	UT_ASSERT(result.is_ok());
	UT_ASSERTeq(expected.compare(result.get_value()), 0);
}

template <bool IsConst>
void verify_value(iterator<IsConst> &it, pmem::kv::string_view expected)
{
	auto result = it.read_range();
	UT_ASSERT(result.is_ok());
	UT_ASSERTeq(expected.compare(result.get_value()), 0);
}

template <bool IsConst>
typename std::enable_if<IsConst, iterator<IsConst>>::type new_iterator(pmem::kv::db &kv)
{
	auto res = kv.new_read_iterator();
	UT_ASSERT(res.is_ok());
	return std::move(res.get_value());
}

template <bool IsConst>
typename std::enable_if<!IsConst, iterator<IsConst>>::type new_iterator(pmem::kv::db &kv)
{
	auto res = kv.new_write_iterator();
	UT_ASSERT(res.is_ok());
	return std::move(res.get_value());
}

template <bool IsConst>
static void verify_keys(iterator<IsConst> &it)
{
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
	});
}
