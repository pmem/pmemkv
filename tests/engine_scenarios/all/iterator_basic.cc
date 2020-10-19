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
		verify_not_found<IsConst>(it);
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
	verify_not_found<IsConst>(it);

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
	verify_not_found<IsConst>(it);

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
	auto it = kv.new_write_iterator();

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<false>(it, p.first);
		verify_value<false>(it, p.second);

		auto res = it.write_range(0, std::numeric_limits<size_t>::max());
		ASSERT_STATUS(res.second, pmem::kv::status::OK);
		for (auto &c : res.first)
			c = 'x';

		/* verify that value not changed before commit */
		auto current = it.read_range(0, std::numeric_limits<size_t>::max());
		ASSERT_STATUS(current.second, pmem::kv::status::OK);
		verify_value<false>(it, p.second);

		it.commit();

		/* check if value changed */
		verify_value<false>(it, std::string(res.first.size(), 'x'));
	});
}

static void write_abort_test(pmem::kv::db &kv)
{
	auto it = kv.new_write_iterator();

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		verify_key<false>(it, p.first);
		verify_value<false>(it, p.second);

		auto res = it.write_range(0, std::numeric_limits<size_t>::max());
		ASSERT_STATUS(res.second, pmem::kv::status::OK);
		for (auto &c : res.first)
			c = 'x';

		/* verify that value not changed before abort */
		auto current = it.read_range(0, std::numeric_limits<size_t>::max());
		ASSERT_STATUS(current.second, pmem::kv::status::OK);
		verify_value<false>(it, p.second);

		it.abort();

		/* check if value not changed after abort */
		verify_value<false>(it, p.second);
	});

	/* check if seek will internally abort transaction */
	ASSERT_STATUS(it.seek(keys[0].first), pmem::kv::status::OK);
	auto res = it.write_range(0, std::numeric_limits<size_t>::max());
	ASSERT_STATUS(res.second, pmem::kv::status::OK);
	for (auto &c : res.first)
		c = 'a';

	it.seek(keys[keys.size() - 1].first);
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
		UT_FATAL("usage: %s engine json_config [test_only_seek_and_write]",
			 argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 seek_test<true>,
				 seek_test<false>,
				 write_test,
				 write_abort_test,
			 });

	if (argc < 4 || std::string(argv[3]).compare("true") != 0) {
		run_engine_tests(argv[1], argv[2],
				 {
					 seek_to_first_test<true>,
					 seek_to_first_test<false>,
					 seek_to_last_test<true>,
					 seek_to_last_test<false>,
				 });
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
