// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

#include <math.h>

/**
 * Generated tests for get_equal_equal_below and count_equal_equal_below methods for
 * sorted engines. get_equal_below method returns all elements in db with keys lesser
 * than or equal to the given key (count returns the number of such records).
 */

static void GetBelowTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some new keys added.
	 * It's NOT suitable to test with custom comparator.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_equal_below(kv, EMPTY_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_basic_keys(kv);

	auto expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"},
				{"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_equal_below(kv, MAX_KEY, 6, kv_sort(expected));

	expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"}, {"B", "4"}};
	verify_get_equal_below(kv, "B", 4, kv_sort(expected));

	expected = kv_list{{"A", "1"}};
	verify_get_equal_below(kv, "AA", 1, kv_sort(expected));

	/* insert new key */
	ASSERT_STATUS(kv.put("BD", "7"), status::OK);

	expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"}, {"B", "4"}};
	verify_get_equal_below(kv, "B", 4, kv_sort(expected));

	verify_get_equal_below(kv, EMPTY_KEY, 0, kv_list());

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_equal_below(kv, "ZZZ", 7, kv_sort(expected));

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_equal_below(kv, "BD", 7, kv_sort(expected));

	/* insert new key with special char in key */
	ASSERT_STATUS(kv.put("记!", "RR"), status::OK);

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_equal_below(kv, "ZZZ", 7, kv_sort(expected));

	expected.emplace_back("记!", "RR");
	verify_get_equal_below(kv, MAX_KEY, 8, kv_sort(expected));

	/* testing C-like API */
	verify_get_equal_below_c(kv, "记!", 8, kv_sort(expected));

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_equal_below_c(kv, "BE", 7, kv_sort(expected));

	CLEAR_KV(kv);
	verify_get_equal_below_c(kv, MAX_KEY, 0, kv_list());

	kv.close();
}

static void GetBelowTest2(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some keys are removed.
	 * This test is using C-like API.
	 * It's NOT suitable to test with custom comparator.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_equal_below_c(kv, MAX_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_ext_keys(kv);

	auto expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},  {"rrr", "4"},
				{"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_equal_below_c(kv, MAX_KEY, 7, kv_sort(expected));

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"}};
	verify_get_equal_below_c(kv, "ccc", 3, kv_sort(expected));

	verify_get_equal_below_c(kv, "a", 0, kv_list());
	verify_get_equal_below_c(kv, EMPTY_KEY, 0, kv_list());

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"}};
	verify_get_equal_below_c(kv, "ddd", 3, kv_sort(expected));

	/* remove one key */
	ASSERT_STATUS(kv.remove("sss"), status::OK);

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"}, {"rrr", "4"}};
	verify_get_equal_below_c(kv, "sss", 4, kv_sort(expected));

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},
			   {"rrr", "4"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_equal_below_c(kv, MAX_KEY, 6, kv_sort(expected));
	verify_get_equal_below_c(kv, "z", 6, kv_sort(expected));

	CLEAR_KV(kv);
	verify_get_equal_below_c(kv, MAX_KEY, 0, kv_list());

	kv.close();
}

static void GetBelowRandTest(std::string engine, pmem::kv::config &&config,
			     const size_t items, const size_t max_key_len)
{
	/**
	 * TEST: Randomly generated keys.
	 */

	/* XXX: add comparator to kv_sort method, perhaps as param */

	/* XXX: to be enabled for Comparator support (in all below test functions) */
	// auto cmp = std::unique_ptr<comparator>(new Comparator());
	// auto s = config.put_comparator(std::move(cmp));
	// ASSERT_STATUS(s, status::OK);

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_equal_below(kv, "randtest", 0, kv_list());

	/* generate keys and put them one at a time */
	std::vector<std::string> keys = gen_rand_keys(items, max_key_len);

	auto expected = kv_list();
	std::string key, value;
	for (size_t i = 0; i < items; i++) {
		value = std::to_string(i);
		key = keys[i];
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		/* verifies all elements */
		verify_get_equal_below(kv, MAX_KEY, i + 1, kv_sort(expected));

		/* verifies all elements */
		auto exp_sorted = kv_sort(expected);
		verify_get_equal_below(kv, exp_sorted[i].first, i + 1,
				       kv_list(exp_sorted.begin(), exp_sorted.end()));

		if (exp_sorted.size() > 1) {
			/* verifies half of elements */
			unsigned half = exp_sorted.size() / 2 + 1;
			verify_get_equal_below(
				kv, exp_sorted[half - 1].first, half,
				kv_list(exp_sorted.begin(), exp_sorted.begin() + half));
		}

		if (exp_sorted.size() > 5) {
			/* verifies first few elements */
			verify_get_equal_below(
				kv, exp_sorted[4].first, 5,
				kv_list(exp_sorted.begin(), exp_sorted.begin() + 5));
		}
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetBelowIncrTest(std::string engine, pmem::kv::config &&config,
			     const size_t max_key_len)
{
	/**
	 * TEST: Generated keys with incremental keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added and checked if get_equal_below returns properly all data.
	 * After initial part of the test, some new keys are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_equal_below(kv, "a_inc", 0, kv_list());

	/* generate keys and put them one at a time */
	std::vector<std::string> keys = gen_incr_keys(max_key_len);
	auto expected = kv_list();
	size_t keys_cnt = charset_size * max_key_len;
	std::string key, value;
	for (size_t i = 0; i < keys_cnt; i++) {
		key = keys[i];
		value = std::to_string(i);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		/* verifies all elements */
		verify_get_equal_below(kv, MAX_KEY, i + 1, kv_sort(expected));

		/* verifies all elements */
		auto exp_sorted = kv_sort(expected);
		verify_get_equal_below(kv, exp_sorted[i].first, i + 1,
				       kv_list(exp_sorted.begin(), exp_sorted.end()));

		if (exp_sorted.size() > 1) {
			/* verifies half of elements */
			unsigned half = exp_sorted.size() / 2 + 1;
			verify_get_equal_below(
				kv, exp_sorted[half - 1].first, half,
				kv_list(exp_sorted.begin(), exp_sorted.begin() + half));
		}
	}

	/* start over with two initial keys */
	CLEAR_KV(kv);
	ASSERT_STATUS(kv.put(MIN_KEY, "init0"), status::OK);
	ASSERT_STATUS(kv.put(MIN_KEY + MIN_KEY, "init1"), status::OK);

	expected = kv_list{{MIN_KEY, "init0"}, {MIN_KEY + MIN_KEY, "init1"}};
	verify_get_equal_below(kv, MAX_KEY, 2, kv_sort(expected));

	/* add keys again */
	keys = gen_incr_keys(max_key_len);
	keys_cnt = charset_size * max_key_len;
	for (size_t i = 0; i < keys_cnt; i++) {
		key = keys[i];
		value = std::to_string(i);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		if (i % 5 == 0) {
			/* verifies all elements */
			verify_get_equal_below(kv, MAX_KEY, i + 3, kv_sort(expected));

			/* verifies all elements */
			auto exp_sorted = kv_sort(expected);
			verify_get_equal_below(
				kv, exp_sorted[i + 2].first, i + 3,
				kv_list(exp_sorted.begin(), exp_sorted.end()));
		}
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetBelowIncrReverseTest(std::string engine, pmem::kv::config &&config,
				    const size_t max_key_len)
{
	/**
	 * TEST: Generated keys with incremental keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added in reverse order and checked if get_equal_below returns properly
	 * all data. After initial part of the test, some keys are deleted and some new
	 * keys are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_equal_below(kv, "&Rev&", 0, kv_list());

	/* generate keys and put them one at a time */
	std::vector<std::string> keys = gen_incr_keys(max_key_len);
	auto expected = kv_list();
	size_t keys_cnt = charset_size * max_key_len;
	std::string key, value;
	for (size_t i = keys_cnt; i > 0; i--) {
		key = keys[i - 1];
		value = std::to_string(i - 1);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		size_t curr_iter = keys_cnt - i + 1;
		/* verifies all elements */
		verify_get_equal_below(kv, MAX_KEY, curr_iter, kv_sort(expected));

		/* verifies all elements */
		auto exp_sorted = kv_sort(expected);
		verify_get_equal_below(kv, exp_sorted[curr_iter - 1].first, curr_iter,
				       kv_list(exp_sorted.begin(), exp_sorted.end()));
	}

	/* delete some keys, add some new keys and check again (using C-like API) */

	/* remove 19th key */
	UT_ASSERT(keys_cnt > 20);
	key = keys[19];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies equal_below 19-th element */
	auto exp_sorted = kv_sort(expected);
	verify_get_equal_below_c(kv, exp_sorted[18].first, 19,
				 kv_list(exp_sorted.begin(), exp_sorted.begin() + 19));

	/* verifies all elements */
	verify_get_equal_below_c(kv, MAX_KEY, keys_cnt, kv_sort(expected));

	/* remove 9th key */
	UT_ASSERT(keys_cnt > 9);
	key = keys[8];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies all elements */
	verify_get_equal_below_c(kv, MAX_KEY, keys_cnt, kv_sort(expected));

	/* remove 3rd key */
	UT_ASSERT(keys_cnt > 3);
	key = keys[2];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies all elements */
	verify_get_equal_below_c(kv, MAX_KEY, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("!@", "!@"), status::OK);
	expected.emplace_back("!@", "!@");
	keys_cnt++;
	verify_get_equal_below_c(kv, MAX_KEY, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("<my_key>", "<my_key>"), status::OK);
	expected.emplace_back("<my_key>", "<my_key>");
	keys_cnt++;
	verify_get_equal_below_c(kv, MAX_KEY, keys_cnt, kv_sort(expected));

	CLEAR_KV(kv);
	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config items max_key_len", argv[0]);

	auto engine = std::string(argv[1]);
	size_t items = std::stoull(argv[3]);
	size_t max_key_len = std::stoull(argv[4]);

	auto seed = unsigned(std::time(0));
	printf("rand seed: %u\n", seed);
	std::srand(seed);

	GetBelowTest(engine, CONFIG_FROM_JSON(argv[2]));
	GetBelowTest2(engine, CONFIG_FROM_JSON(argv[2]));
	GetBelowRandTest(engine, CONFIG_FROM_JSON(argv[2]), items, max_key_len);
	GetBelowIncrTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
	GetBelowIncrReverseTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
