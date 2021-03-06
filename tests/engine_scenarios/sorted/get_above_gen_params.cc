// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

/**
 * Generated tests for get_above and count_above methods for sorted engines.
 * get_above method returns all elements in db with keys greater than the given key
 * (count returns the number of such records).
 */

static void GetAboveTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some new keys added.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_above(kv, EMPTY_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_basic_keys(kv);

	auto expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"},
				{"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_above(kv, EMPTY_KEY, 6, kv_sort(expected));

	expected = kv_list{{"BB", "5"}, {"BC", "6"}};
	verify_get_above(kv, "B", 2, kv_sort(expected));

	/* insert new key */
	ASSERT_STATUS(kv.put("BD", "7"), status::OK);

	expected = kv_list{{"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_above(kv, "B", 3, kv_sort(expected));

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_above(kv, EMPTY_KEY, 7, kv_sort(expected));

	verify_get_above(kv, "ZZZ", 0, kv_list());

	expected = kv_list{{"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_above(kv, "BA", 3, kv_sort(expected));

	/* insert new key with special char in key */
	ASSERT_STATUS(kv.put("记!", "RR"), status::OK);

	/* testing C-like API */
	expected = kv_list{{"BB", "5"}, {"BC", "6"}, {"BD", "7"}, {"记!", "RR"}};
	verify_get_above_c(kv, "B", 4, kv_sort(expected));

	verify_get_above_c(kv, "记!", 0, kv_list());

	CLEAR_KV(kv);
	verify_get_above_c(kv, MIN_KEY, 0, kv_list());

	kv.close();
}

static void GetAboveReverseTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some new keys added.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_above(kv, EMPTY_KEY, 0, {});
	verify_get_above_c(kv, MAX_KEY, 0, {});

	/* insert bunch of keys */
	add_basic_keys(kv);

	verify_get_above(kv, EMPTY_KEY, 0, {});

	auto expected = kv_list{{"AC", "3"}, {"AB", "2"}, {"A", "1"}};
	verify_get_above(kv, "B", 3, expected);

	/* insert new key */
	ASSERT_STATUS(kv.put("BD", "7"), status::OK);

	expected = kv_list{{"AC", "3"}, {"AB", "2"}, {"A", "1"}};
	verify_get_above(kv, "B", 3, expected);

	expected = kv_list{{"BD", "7"}, {"BC", "6"}, {"BB", "5"}, {"B", "4"},
			   {"AC", "3"}, {"AB", "2"}, {"A", "1"}};
	verify_get_above(kv, MAX_KEY, 7, expected);

	/* insert new key with special char in key */
	ASSERT_STATUS(kv.put("记!", "RR"), status::OK);

	/* testing C-like API */
	expected = kv_list{{"AC", "3"}, {"AB", "2"}, {"A", "1"}};
	verify_get_above_c(kv, "B", 3, expected);

	expected = kv_list{{"BD", "7"}, {"BC", "6"}, {"BB", "5"}, {"B", "4"},
			   {"AC", "3"}, {"AB", "2"}, {"A", "1"}};
	verify_get_above_c(kv, "记!", 7, expected);

	CLEAR_KV(kv);
	verify_get_above_c(kv, MIN_KEY, 0, {});
	verify_get_above_c(kv, MAX_KEY, 0, {});

	kv.close();
}

static void GetAboveTest2(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some keys are removed.
	 * This test is using C-like API.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_above_c(kv, MIN_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_ext_keys(kv);

	auto expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},  {"rrr", "4"},
				{"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_above_c(kv, MIN_KEY, 7, kv_sort(expected));

	expected = kv_list{{"rrr", "4"}, {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_above_c(kv, "ccc", 4, kv_sort(expected));

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},  {"rrr", "4"},
			   {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_above_c(kv, "a", 7, kv_sort(expected));

	expected = kv_list{{"rrr", "4"}, {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_above_c(kv, "ddd", 4, kv_sort(expected));

	/* remove one key */
	ASSERT_STATUS(kv.remove("sss"), status::OK);

	expected = kv_list{{"rrr", "4"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_above_c(kv, "ddd", 3, kv_sort(expected));

	verify_get_above_c(kv, "z", 0, kv_list());

	CLEAR_KV(kv);
	verify_get_above_c(kv, MAX_KEY, 0, kv_list());

	kv.close();
}

static void GetAboveRandTest(std::string engine, pmem::kv::config &&config,
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
	verify_get_above(kv, "randtest", 0, kv_list());

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
		verify_get_above(kv, MIN_KEY, i + 1, kv_sort(expected));

		/* verifies elements above the first one */
		auto exp_sorted = kv_sort(expected);
		verify_get_above(kv, exp_sorted[0].first, i,
				 kv_list(exp_sorted.begin() + 1, exp_sorted.end()));

		if (exp_sorted.size() > 1) {
			/* verifies half of elements */
			unsigned half = exp_sorted.size() / 2;
			verify_get_above(
				kv, exp_sorted[half - 1].first, exp_sorted.size() - half,
				kv_list(exp_sorted.begin() + half, exp_sorted.end()));
		}

		if (exp_sorted.size() > 5) {
			/* verifies last few elements */
			verify_get_above(kv, exp_sorted[exp_sorted.size() - 5].first, 4,
					 kv_list(exp_sorted.end() - 4, exp_sorted.end()));
		}
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetAboveIncrTest(std::string engine, pmem::kv::config &&config,
			     const size_t max_key_len)
{
	/**
	 * TEST: Generated keys with incremental keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added and checked if get_above returns properly all data.
	 * After initial part of the test, some new keys are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_above(kv, "a_inc", 0, kv_list());

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
		verify_get_above(kv, MIN_KEY, i + 1, kv_sort(expected));

		/* verifies elements above the first one */
		auto exp_sorted = kv_sort(expected);
		verify_get_above(kv, exp_sorted[0].first, i,
				 kv_list(exp_sorted.begin() + 1, exp_sorted.end()));

		if (exp_sorted.size() > 1) {
			/* verifies half of elements */
			unsigned half = exp_sorted.size() / 2;
			verify_get_above(
				kv, exp_sorted[half - 1].first, exp_sorted.size() - half,
				kv_list(exp_sorted.begin() + half, exp_sorted.end()));
		}
	}

	/* start over with two initial keys */
	CLEAR_KV(kv);
	ASSERT_STATUS(kv.put(MAX_KEY, "init0"), status::OK);
	ASSERT_STATUS(kv.put(MAX_KEY + MAX_KEY, "init1"), status::OK);

	expected = kv_list{{MAX_KEY, "init0"}, {MAX_KEY + MAX_KEY, "init1"}};
	verify_get_above(kv, MIN_KEY, 2, kv_sort(expected));

	/* add keys again */
	keys = gen_incr_keys(max_key_len);
	keys_cnt = charset_size * max_key_len;
	for (size_t i = 0; i < keys_cnt; i++) {
		key = keys[i];
		value = std::to_string(i);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		/* verifies all elements */
		verify_get_above(kv, MIN_KEY, i + 3, kv_sort(expected));

		/* verifies elements from 2nd to last */
		auto exp_sorted = kv_sort(expected);
		verify_get_above(kv, exp_sorted[0].first, i + 2,
				 kv_list(exp_sorted.begin() + 1, exp_sorted.end()));
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetAboveIncrReverseTest(std::string engine, pmem::kv::config &&config,
				    const size_t max_key_len)
{
	/**
	 * TEST: Generated keys with incremental keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added in reverse order and checked if get_above returns properly all
	 * data. After initial part of the test, some keys are deleted and some new keys
	 * are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_above(kv, "&Rev&", 0, kv_list());

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

		/* verifies all elements */
		verify_get_above(kv, MIN_KEY, keys_cnt - i + 1, kv_sort(expected));

		/* verifies elements above the first one */
		auto exp_sorted = kv_sort(expected);
		verify_get_above(kv, exp_sorted[0].first, keys_cnt - i,
				 kv_list(exp_sorted.begin() + 1, exp_sorted.end()));
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

	/* verifies above 11th element */
	auto exp_sorted = kv_sort(expected);
	verify_get_above_c(kv, exp_sorted[10].first, keys_cnt - 11,
			   kv_list(exp_sorted.begin() + 11, exp_sorted.end()));

	/* verifies all elements */
	verify_get_above_c(kv, MIN_KEY, keys_cnt, kv_sort(expected));

	/* remove 9th key */
	UT_ASSERT(keys_cnt > 9);
	key = keys[8];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies all elements */
	verify_get_above_c(kv, MIN_KEY, keys_cnt, kv_sort(expected));

	/* remove 3rd key */
	UT_ASSERT(keys_cnt > 3);
	key = keys[2];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies all elements */
	verify_get_above_c(kv, MIN_KEY, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("!@", "!@"), status::OK);
	expected.emplace_back("!@", "!@");
	keys_cnt++;
	verify_get_above_c(kv, MIN_KEY, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("<my_key>", "<my_key>"), status::OK);
	expected.emplace_back("<my_key>", "<my_key>");
	keys_cnt++;
	verify_get_above_c(kv, MIN_KEY, keys_cnt, kv_sort(expected));

	CLEAR_KV(kv);
	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 6)
		UT_FATAL("usage: %s engine json_config comparator items max_key_len",
			 argv[0]);

	auto engine = std::string(argv[1]);
	auto comparator = std::string(argv[3]);
	size_t items = std::stoull(argv[4]);
	size_t max_key_len = std::stoull(argv[5]);

	auto seed = unsigned(std::time(0));
	printf("rand seed: %u\n", seed);
	std::srand(seed);

	if (comparator == "default") {
		GetAboveTest(engine, CONFIG_FROM_JSON(argv[2]));
		GetAboveTest2(engine, CONFIG_FROM_JSON(argv[2]));
		GetAboveRandTest(engine, CONFIG_FROM_JSON(argv[2]), items, max_key_len);
		GetAboveIncrTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
		GetAboveIncrReverseTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
	} else if (comparator == "reverse") {
		auto make_config = [&] {
			auto cfg = CONFIG_FROM_JSON(argv[2]);
			ASSERT_STATUS(cfg.put_comparator(reverse_comparator{}),
				      status::OK);

			return cfg;
		};

		GetAboveReverseTest(engine, make_config());
	} else
		UT_FATAL("Unknown comparator");
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
