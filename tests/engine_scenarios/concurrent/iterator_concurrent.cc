// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../all/iterator.hpp"
#include "unittest.hpp"

#include <vector>

static void init_keys(pmem::kv::db &kv, size_t size)
{
	for (size_t i = 0; i < size; i++)
		ASSERT_STATUS(kv.put(std::to_string(i), std::string(20 + i, 'x')),
			      pmem::kv::status::OK);
}

static void verify_kv(pmem::kv::db &kv, size_t size)
{
	auto it = kv.new_read_iterator();
	for (size_t i = 0; i < size; i++) {
		ASSERT_STATUS(it.seek(std::to_string(i)), pmem::kv::status::OK);
		verify_value<true>(it, std::string(20 + i, 'x'));
	}
}

static void concurrent_write(size_t threads_number, pmem::kv::db &kv)
{
	const size_t n = threads_number * 10;
	init_keys(kv, n);

	parallel_exec(threads_number + 1, [&](size_t thread_id) {
		/* check consistency */
		if (thread_id == threads_number) {
			auto it = kv.new_read_iterator();
			for (size_t i = 0; i < n; i++) {
				ASSERT_STATUS(it.seek(std::to_string(i)),
					      pmem::kv::status::OK);

				auto res = it.read_range(
					0, std::numeric_limits<size_t>::max());
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				auto value =
					std::string(res.first.begin(), res.first.size());

				std::string possible_values[4] = {
					std::string(20 + i, 'x'),
					std::string(10, 'b') + std::string(10 + i, 'x'),
					std::string(10, 'x') + std::string(10 + i, 'a'),
					std::string(10, 'b') + std::string(10 + i, 'a'),
				};

				UT_ASSERT(
					std::any_of(possible_values, possible_values + 4,
						    [&](std::string s) {
							    return s.compare(value) == 0;
						    }));
			}

			return;
		}

		auto it = kv.new_write_iterator();
		for (size_t i = thread_id / 2; i < n; i += threads_number / 2) {
			ASSERT_STATUS(it.seek(std::to_string(i)), pmem::kv::status::OK);
			/* write 'a' to chars from 10 to end */
			if (thread_id % 2) {
				auto res = it.write_range(
					10, std::numeric_limits<size_t>::max());
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				for (auto &c : res.first)
					c = 'a';
			}
			/* write 'b' to chars from begin to 10 */
			else {
				auto res = it.write_range(0, 10);
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				for (auto &c : res.first)
					c = 'b';
			}
			ASSERT_STATUS(it.commit(), pmem::kv::status::OK);
		}
	});

	auto it = kv.new_read_iterator();
	for (size_t i = 0; i < n; i++) {
		ASSERT_STATUS(it.seek(std::to_string(i)), pmem::kv::status::OK);
		verify_value<true>(it, std::string(10, 'b') + std::string(10 + i, 'a'));
	}
}

static void concurrent_write_abort(size_t threads_number, pmem::kv::db &kv)
{
	const size_t n = threads_number * 10;
	init_keys(kv, n);

	parallel_exec(threads_number + 1, [&](size_t thread_id) {
		if (thread_id == threads_number) {
			verify_kv(kv, n);
			return;
		}

		auto it = kv.new_write_iterator();
		for (size_t i = thread_id / 2; i < n; i += threads_number) {
			it.seek(std::to_string(i));
			if (thread_id % 2) {
				auto res = it.write_range(
					10, std::numeric_limits<size_t>::max());
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				for (auto &c : res.first)
					c = 'a';
			} else {
				auto res = it.write_range(0, 10);
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				for (auto &c : res.first)
					c = 'b';
			}
			it.abort();
		}
	});

	verify_kv(kv, n);
}

/* only for sorted engines */
static void concurrent_write_sorted(size_t threads_number, pmem::kv::db &kv)
{
	const size_t n = threads_number * 10;
	init_keys(kv, n);

	parallel_exec(threads_number + 1, [&](size_t thread_id) {
		/* check consistency */
		if (thread_id == threads_number) {
			auto it = kv.new_read_iterator();
			ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

			do {
				auto key = it.key();
				ASSERT_STATUS(key.second, pmem::kv::status::OK);
				auto key_as_number =
					static_cast<size_t>(std::stoi(key.first.data()));

				auto res = it.read_range(
					0, std::numeric_limits<size_t>::max());
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				auto value =
					std::string(res.first.begin(), res.first.size());

				std::string possible_values[4] = {
					std::string(20 + key_as_number, 'x'),
					std::string(10, 'b') +
						std::string(10 + key_as_number, 'x'),
					std::string(10, 'x') +
						std::string(10 + key_as_number, 'a'),
					std::string(10, 'b') +
						std::string(10 + key_as_number, 'a'),
				};

				UT_ASSERT(
					std::any_of(possible_values, possible_values + 4,
						    [&](std::string s) {
							    return s.compare(value) == 0;
						    }));
			} while (it.next() == pmem::kv::status::OK);

			return;
		}

		auto it = kv.new_write_iterator();
		ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

		do {
			/* write 'a' to chars from 10 to end */
			if (thread_id % 2) {
				auto res = it.write_range(
					10, std::numeric_limits<size_t>::max());
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				for (auto &c : res.first)
					c = 'a';
			}
			/* write 'b' to chars from begin to 10 */
			else {
				auto res = it.write_range(0, 10);
				ASSERT_STATUS(res.second, pmem::kv::status::OK);
				for (auto &c : res.first)
					c = 'b';
			}
			ASSERT_STATUS(it.commit(), pmem::kv::status::OK);
		} while (it.next() == pmem::kv::status::OK);
	});

	auto it = kv.new_read_iterator();
	for (size_t i = 0; i < n; i++) {
		ASSERT_STATUS(it.seek(std::to_string(i)), pmem::kv::status::OK);
		verify_value<true>(it, std::string(10, 'b') + std::string(10 + i, 'a'));
	}
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 4)
		UT_FATAL("usage: %s engine json_config threads [is_sorted]", argv[0]);

	size_t threads_number = std::stoull(argv[3]);
	run_engine_tests(argv[1], argv[2],
			 {
				 std::bind(concurrent_write_abort, threads_number, _1),
				 std::bind(concurrent_write, threads_number, _1),
			 });

	/* only for sorted engines */
	if (argc > 4 && std::string(argv[4]).compare("true") == 0) {
		run_engine_tests(
			argv[1], argv[2],
			{
				std::bind(concurrent_write_sorted, threads_number, _1),
			});
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
