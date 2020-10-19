// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../common/unittest.hpp"

template <bool IsConst>
using iterator = typename std::conditional<IsConst, pmem::kv::db::read_iterator,
					   pmem::kv::db::write_iterator>::type;

using pair = std::pair<std::string, std::string>;

template <bool IsConst = true>
using key_result = std::pair<typename std::conditional<IsConst, pmem::kv::string_view,
						       pmem::kv::db::accessor>::type,
			     pmem::kv::status>;

static std::vector<pair> keys{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},	 {"rrr", "4"},
			      {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};

void insert_keys(pmem::kv::db &kv)
{
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(kv.put(p.first, p.second), pmem::kv::status::OK);
	});
}

void verify_key(key_result<> result, pmem::kv::string_view expected)
{
	ASSERT_STATUS(result.second, pmem::kv::status::OK);
	UT_ASSERTeq(expected.compare(result.first), 0);
}

template <bool IsConst>
typename std::enable_if<IsConst>::type verify_value(key_result<IsConst> result,
						    pmem::kv::string_view expected)
{
	ASSERT_STATUS(result.second, pmem::kv::status::OK);
	UT_ASSERTeq(expected.compare(result.first), 0);
}

template <bool IsConst>
typename std::enable_if<!IsConst>::type verify_value(key_result<IsConst> result,
						     pmem::kv::string_view expected)
{
	ASSERT_STATUS(result.second, pmem::kv::status::OK);
	auto read_res = result.first.read_range(0, std::numeric_limits<size_t>::max());
	ASSERT_STATUS(read_res.second, pmem::kv::status::OK);
	UT_ASSERTeq(expected.compare(pmem::kv::string_view{read_res.first.begin()}), 0);
}

template <bool IsConst>
void verify_not_found(iterator<IsConst> &it)
{
	ASSERT_STATUS(it.key().second, pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(it.value().second, pmem::kv::status::NOT_FOUND);
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
