// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../iterator.hpp"

/**
 * Test methods available only in sorted engines' iterators.
 */

template <bool IsConst>
static void seek_lower_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower(p.first), pmem::kv::status::NOT_FOUND);
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
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower_eq(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
	});

	/* check with not equal elements */
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower_eq(p.first + "aa"), pmem::kv::status::OK);
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
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher_eq(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
	});

	/* check with not equal elements */
	auto prev = std::string("aa");
	std::for_each(keys.begin(), keys.end() - 1, [&](pair p) {
		ASSERT_STATUS(it.seek_higher_eq(prev), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);

		prev = p.first + "aa";
	});
}

template <bool IsConst>
static void next_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	std::for_each(keys.begin(), keys.end() - 1, [&](pair p) {
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
		ASSERT_STATUS(it.is_next(), pmem::kv::status::OK);
		ASSERT_STATUS(it.next(), pmem::kv::status::OK);
	});

	verify_key<IsConst>(it, (--keys.end())->first);
	verify_value<IsConst>(it, (--keys.end())->second);
	ASSERT_STATUS(it.is_next(), pmem::kv::status::NOT_FOUND);
	ASSERT_STATUS(it.next(), pmem::kv::status::NOT_FOUND);
}

template <bool IsConst>
static void prev_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

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

template <bool IsConst>
static void seek_to_first_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::NOT_FOUND);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	auto first_key = keys.begin();
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);
		verify_key<IsConst>(it, first_key->first);
		verify_value<IsConst>(it, first_key->second);
	});
}

template <bool IsConst>
static void seek_to_last_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::NOT_FOUND);

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);

	auto last_key = --keys.end();
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);
		verify_key<IsConst>(it, last_key->first);
		verify_value<IsConst>(it, last_key->second);
	});
}

static void seek_to_first_write_test(pmem::kv::db &kv)
{
	auto it = new_iterator<false>(kv);

	insert_keys(kv);

	/* check if seek_to_first() will internally abort transaction */
	it.seek(keys.back().first);
	auto res = it.write_range();
	UT_ASSERT(res.is_ok());
	for (auto &c : res.get_value())
		c = 'a';

	it.seek_to_first();
	it.commit();

	verify_keys<false>(it);

	/* write something after seek_to_first() */
	it.seek_to_first();
	auto res2 = it.write_range();
	UT_ASSERT(res2.is_ok());
	for (auto &c : res2.get_value())
		c = 'o';

	it.commit();

	verify_value<false>(it, std::string(keys.front().second.size(), 'o'));
}

static void seek_to_last_write_test(pmem::kv::db &kv)
{
	auto it = new_iterator<false>(kv);

	insert_keys(kv);

	/* check if seek_to_last will internally abort transaction */
	it.seek(keys.front().first);
	auto res = it.write_range();
	UT_ASSERT(res.is_ok());
	for (auto &c : res.get_value())
		c = 'a';

	it.seek_to_last();
	it.commit();

	verify_keys<false>(it);

	/* write something after seek_to_last() */
	it.seek_to_last();
	auto res2 = it.write_range();
	UT_ASSERT(res2.is_ok());
	for (auto &c : res2.get_value())
		c = 'o';

	it.commit();

	verify_value<false>(it, std::string(keys.back().second.size(), 'o'));
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
				 seek_to_first_test<true>,
				 seek_to_first_test<false>,
				 seek_to_first_write_test,
			 });

	/* check if iterator supports prev and seek_to_last methods */
	if (argc < 4 || std::string(argv[3]).compare("false") != 0)
		run_engine_tests(argv[1], argv[2],
				 {
					 prev_test<true>,
					 prev_test<false>,
					 seek_to_last_test<true>,
					 seek_to_last_test<false>,
					 seek_to_last_write_test,
				 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
