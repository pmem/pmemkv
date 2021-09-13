// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include <ctime>
#include <unordered_set>

/**
 * Tests concurrency with parallel data read and removal. Data is generated with
 * parametrized thread count, database elements count and max key length.
 */

using namespace pmem::kv;

void generate_keys(std::vector<std::string> &keys, const size_t max_key_len,
		   const size_t cnt)
{
	const char charset[] = "abcdefghijklmnopqrstuvwxyz"
			       "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
			       "!@#$%^&*()_+,./<>?'\"`~;:[]{}\\|";
	const size_t charset_size = sizeof(charset);

	std::unordered_set<std::string> unique_keys;

	size_t k = 0;
	while (k < cnt) {
		/* various lengths of key, min: 1 */
		size_t key_len = 1 + ((size_t)rand() % max_key_len);
		std::string key;
		key.reserve(key_len);

		for (size_t i = 0; i < key_len; i++) {
			key.push_back(charset[(size_t)rand() % charset_size]);
		}

		std::string generated_key = entry_from_string(key);

		/* if key doesn't already exists, insert */
		if (unique_keys.insert(generated_key).second)
			++k;
	}

	keys = std::vector<std::string>(unique_keys.begin(), unique_keys.end());
}

static void MultithreadedTestRemoveDataAside(const size_t threads_number,
					     const size_t thread_items,
					     const size_t max_key_len, pmem::kv::db &kv)
{
	/**
	 * TEST: reads initial data in parallel while operating on other (generated) data.
	 */
	size_t initial_count = 128;
	/* put initial data, which won't be modified */
	for (size_t i = 0; i < initial_count; i++) {
		std::string key = entry_from_number(i, "in_");
		std::string val = entry_from_number(i, "in_", "!");
		ASSERT_STATUS(kv.put(key, val), status::OK);
	}

	std::vector<std::string> keys;
	auto keys_cnt = threads_number * thread_items;
	generate_keys(keys, max_key_len, keys_cnt);

	/* test parallelly adding data (+ read initial data) */
	parallel_exec(threads_number + 1, [&](size_t thread_id) {
		if (thread_id == threads_number) {
			for (size_t i = 0; i < initial_count; i++) {
				std::string key = entry_from_number(i, "in_");
				std::string val = entry_from_number(i, "in_", "!");
				std::string value;
				ASSERT_STATUS(kv.get(key, &value), status::OK);
				UT_ASSERT(value == val);
			}
			return;
		}
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			ASSERT_STATUS(kv.put(keys[i], keys[i]), status::OK);
		}
		for (auto i = begin; i < end; i++) {
			std::string value;
			ASSERT_STATUS(kv.get(keys[i], &value), status::OK);
			UT_ASSERT(value == keys[i]);
		}
	});
	ASSERT_SIZE(kv, initial_count + keys_cnt);

	/* test parallelly removing data (+ read initial data) */
	parallel_exec(threads_number + 1, [&](size_t thread_id) {
		if (thread_id == threads_number) {
			for (size_t i = 0; i < initial_count; i++) {
				std::string key = entry_from_number(i, "in_");
				std::string val = entry_from_number(i, "in_", "!");
				std::string value;
				ASSERT_STATUS(kv.get(key, &value), status::OK);
				UT_ASSERT(value == val);
			}
			return;
		}
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			ASSERT_STATUS(kv.remove(keys[i]), status::OK);
		}
	});
	ASSERT_SIZE(kv, initial_count);

	/* get initial data and confirm it's unmodified */
	for (size_t i = 0; i < initial_count; i++) {
		std::string key = entry_from_number(i, "in_");
		std::string val = entry_from_number(i, "in_", "!");
		std::string value;
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == val);
	}
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 6)
		UT_FATAL("usage: %s engine json_config threads items max_key_len",
			 argv[0]);

	auto seed = unsigned(std::time(0));
	printf("rand seed: %u\n", seed);
	std::srand(seed);

	size_t threads_number = std::stoull(argv[3]);
	size_t thread_items = std::stoull(argv[4]);
	size_t max_key_len = std::stoull(argv[5]);
	run_engine_tests(argv[1], argv[2],
			 {
				 std::bind(MultithreadedTestRemoveDataAside,
					   threads_number, thread_items, max_key_len, _1),
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
