// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <vector>

#include "iterator.hpp"

template <bool IsConst>
static void seek_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::NOT_FOUND);
		verify_not_found<IsConst>(it);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key(it.key(), p.first);
		verify_value<IsConst>(it.value(), p.second);
	});
}

template <bool IsConst>
static void seek_to_first_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::NOT_FOUND);
	verify_not_found<IsConst>(it);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	auto first_key = keys.begin();
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);
		verify_key(it.key(), first_key->first);
		verify_value<IsConst>(it.value(), first_key->second);
	});
}

template <bool IsConst>
static void seek_to_last_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::NOT_FOUND);
	verify_not_found<IsConst>(it);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);

	auto last_key = --keys.end();
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);
		verify_key(it.key(), last_key->first);
		verify_value<IsConst>(it.value(), last_key->second);
	});
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {seek_test<true>, seek_test<false>, seek_to_first_test<true>,
			  seek_to_first_test<false>, seek_to_last_test<true>,
			  seek_to_last_test<false>});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
