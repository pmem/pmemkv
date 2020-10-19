// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../all/iterator.hpp"

/**
 * Test basic methods available only in sorted engines's iterators.
 */

template <bool IsConst>
static void seek_lower_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower(p.first), pmem::kv::status::NOT_FOUND);
		verify_not_found<IsConst>(it);
	});

	insert_keys(kv);

	ASSERT_STATUS(it.seek_lower(keys[0].first), pmem::kv::status::NOT_FOUND);

	auto prev_key = keys.begin();
	std::for_each(keys.begin() + 1, keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, prev_key->first);
		verify_value<IsConst>(it, (prev_key++)->second);
	});
}

template <bool IsConst>
static void seek_lower_eq_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower_eq(p.first), pmem::kv::status::NOT_FOUND);
		verify_not_found<IsConst>(it);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower_eq(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
	});
}

template <bool IsConst>
static void seek_higher_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher(p.first), pmem::kv::status::NOT_FOUND);
		verify_not_found<IsConst>(it);
	});

	insert_keys(kv);

	auto next_key = keys.begin() + 1;
	std::for_each(keys.begin(), keys.end() - 1, [&](pair p) {
		ASSERT_STATUS(it.seek_higher(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, next_key->first);
		verify_value<IsConst>(it, (next_key++)->second);
	});

	ASSERT_STATUS(it.seek_higher(keys[keys.size() - 1].first),
		      pmem::kv::status::NOT_FOUND);
}

template <bool IsConst>
static void seek_higher_eq_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher_eq(p.first), pmem::kv::status::NOT_FOUND);
		verify_not_found<IsConst>(it);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher_eq(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
	});
}

template <bool IsConst>
static void next_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	ASSERT_STATUS(it.next(), pmem::kv::status::NOT_FOUND);
	verify_not_found<IsConst>(it);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	std::for_each(keys.begin(), keys.end() - 1, [&](pair p) {
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
		ASSERT_STATUS(it.next(), pmem::kv::status::OK);
	});

	verify_key<IsConst>(it, (--keys.end())->first);
	verify_value<IsConst>(it, (--keys.end())->second);
	ASSERT_STATUS(it.next(), pmem::kv::status::NOT_FOUND);
}

template <bool IsConst>
static void prev_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	ASSERT_STATUS(it.prev(), pmem::kv::status::NOT_FOUND);
	verify_not_found<IsConst>(it);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);

	std::for_each(keys.rbegin(), keys.rend() - 1, [&](pair p) {
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
		ASSERT_STATUS(it.prev(), pmem::kv::status::OK);
	});

	verify_key<IsConst>(it, keys.begin()->first);
	verify_value<IsConst>(it, keys.begin()->second);
	ASSERT_STATUS(it.prev(), pmem::kv::status::NOT_FOUND);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config [if_test_prev]", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 seek_lower_test<true>,
				 seek_lower_test<false>,
				 seek_lower_eq_test<true>,
				 seek_lower_eq_test<false>,
				 seek_higher_test<true>,
				 seek_higher_test<false>,
				 seek_higher_eq_test<true>,
				 seek_higher_eq_test<false>,
				 next_test<true>,
				 next_test<false>,
			 });

	/* check if iterator supports prev method */
	if (argc < 4 || std::string(argv[3]).compare("false") != 0)
		run_engine_tests(argv[1], argv[2],
				 {
					 prev_test<true>,
					 prev_test<false>,
				 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
