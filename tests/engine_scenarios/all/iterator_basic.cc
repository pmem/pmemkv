// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * Test basic methods available in iterators (sorted and unsorted engines).
 */

#include <vector>

#include "iterator.hpp"

template <bool IsConst>
static void seek_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<IsConst>(it, p.first);
		verify_value<IsConst>(it, p.second);
	});
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

/* only for non const (write) iterators */
static void write_test(pmem::kv::db &kv)
{
	auto it = new_iterator<false>(kv);

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<false>(it, p.first);
		verify_value<false>(it, p.second);

		auto res = it.write_range();
		UT_ASSERT(res.is_ok());
		for (auto &c : res.get_value())
			c = 'x';

		/* verify that value has not changed before commit */
		verify_value<false>(it, p.second);

		it.commit();

		/* check if value has changed */
		verify_value<false>(it, std::string(res.get_value().size(), 'x'));
	});

	/* write only two last characters */
	auto last = keys[keys.size() - 1];
	it.seek(last.first);
	auto res = it.write_range(last.second.size() - 2,
				  std::numeric_limits<size_t>::max());
	UT_ASSERT(res.is_ok());
	for (auto &c : res.get_value())
		c = 'a';
	it.commit();

	verify_value<false>(
		it, std::string(last.second.size() - 2, 'x') + std::string(2, 'a'));
}

static void write_abort_test(pmem::kv::db &kv)
{
	auto it = new_iterator<false>(kv);

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<false>(it, p.first);
		verify_value<false>(it, p.second);

		auto res = it.write_range();
		UT_ASSERT(res.is_ok());
		for (auto &c : res.get_value())
			c = 'x';

		/* verify that value has not changed before abort */
		verify_value<false>(it, p.second);

		it.abort();

		/* check if value has not changed after abort */
		verify_value<false>(it, p.second);
	});

	/* check if seek will internally abort transaction */
	ASSERT_STATUS(it.seek(keys.front().first), pmem::kv::status::OK);
	auto res = it.write_range();
	UT_ASSERT(res.is_ok());
	for (auto &c : res.get_value())
		c = 'a';

	it.seek(keys.back().first);
	it.commit();

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<false>(it, p.first);
		verify_value<false>(it, p.second);
	});
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL(
			"usage: %s engine json_config [test_seek_to_first] [test_seek_to_last]",
			argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 seek_test<true>,
				 seek_test<false>,
				 write_test,
				 write_abort_test,
			 });

	if (argc < 4 || std::string(argv[3]).compare("true") == 0) {
		run_engine_tests(argv[1], argv[2],
				 {
					 seek_to_first_test<true>,
					 seek_to_first_test<false>,

				 });
	}

	if (argc < 5 || std::string(argv[4]).compare("true") == 0) {
		run_engine_tests(argv[1], argv[2],
				 {
					 seek_to_last_test<true>,
					 seek_to_last_test<false>,
				 });
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
