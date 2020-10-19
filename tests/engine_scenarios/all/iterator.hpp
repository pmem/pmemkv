// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

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
	ASSERT_STATUS(result.second, pmem::kv::status::OK);
	UT_ASSERTeq(expected.compare(result.first), 0);
}

template <bool IsConst>
void verify_value(iterator<IsConst> &it, pmem::kv::string_view expected)
{
	auto result = it.read_range(0, std::numeric_limits<size_t>::max());
	ASSERT_STATUS(result.second, pmem::kv::status::OK);
	UT_ASSERTeq(expected.compare(pmem::kv::string_view{result.first.begin()}), 0);
}

template <bool IsConst>
void verify_not_found(iterator<IsConst> &it)
{
	ASSERT_STATUS(it.key().second, pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(it.read_range(0, std::numeric_limits<size_t>::max()).second,
		      pmem::kv::status::NOT_FOUND);
}

template <bool IsConst>
typename std::enable_if<IsConst, iterator<IsConst>>::type new_iterator(pmem::kv::db &kv)
{
	return kv.new_read_iterator();
}

template <bool IsConst>
typename std::enable_if<!IsConst, iterator<IsConst>>::type new_iterator(pmem::kv::db &kv)
{
	return kv.new_write_iterator();
}
