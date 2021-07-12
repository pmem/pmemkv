// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/**
 * Test basic methods available in iterators (sorted and unsorted engines).
 */

#include <vector>

#include "../iterator.hpp"

template <bool IsConst>
static void seek_test(pmem::kv::db &kv)
{
	auto it = new_iterator<IsConst>(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	verify_keys<IsConst>(it);
}

/* only for non const (write) iterators */
static void write_test(pmem::kv::db &kv)
{
	insert_keys(kv);

	{
		auto it = new_iterator<false>(kv);

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
		auto last = keys.back();
		it.seek(last.first);
		auto res = it.write_range(last.second.size() - 2,
					  std::numeric_limits<size_t>::max());
		UT_ASSERT(res.is_ok());
		for (auto &c : res.get_value())
			c = 'a';
		it.commit();

		verify_value<false>(it,
				    std::string(last.second.size() - 2, 'x') +
					    std::string(2, 'a'));

		/* write only two first characters */
		it.seek(last.first);
		auto res2 = it.write_range(0, 2);
		UT_ASSERT(res2.is_ok());
		for (auto &c : res2.get_value())
			c = 'b';
		it.commit();

		verify_value<false>(it, std::string(2, 'b') + std::string(2, 'a'));

		/* write only two elements from the second position */
		it.seek(last.first);
		auto res3 = it.write_range(1, 2);
		UT_ASSERT(res3.is_ok());
		for (auto &c : res3.get_value())
			c = 'c';
		it.commit();

		verify_value<false>(it, "bcca");
	}

	/* check if a read iterator sees modifications */
	auto r_it = new_iterator<true>(kv);
	r_it.seek(keys.back().first);
	verify_value<true>(r_it, "bcca");
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

	verify_keys<false>(it);
}

static void zeroed_key_test(pmem::kv::db &kv)
{
	auto element = pmem::kv::string_view("z\0z", 3);
	UT_ASSERTeq(element.size(), 3);

	auto s = kv.put(element, "val1");
	ASSERT_STATUS(s, pmem::kv::status::OK);

	s = kv.exists(element);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	auto it = new_iterator<true>(kv);
	s = it.seek(element);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	verify_key<true>(it, element);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 seek_test<true>,
				 seek_test<false>,
				 write_test,
				 write_abort_test,
				 zeroed_key_test,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
