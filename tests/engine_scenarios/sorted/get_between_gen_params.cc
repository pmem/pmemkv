// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

/**
 * Basic + generated tests for get_between and count_between methods for sorted engines.
 * get_between method returns all elements in db with keys greater than key1
 * and lesser than key2 (count returns the number of such records).
 */

static void GetBetweenTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some new keys added.
	 * It's NOT suitable to test with custom comparator.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_between(kv, MIN_KEY, MAX_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_basic_keys(kv);

	auto expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"},
				{"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_between(kv, EMPTY_KEY, MAX_KEY, 6, kv_sort(expected));

	expected = kv_list{{"AB", "2"}, {"AC", "3"}};
	verify_get_between(kv, "A", "B", 2, kv_sort(expected));

	expected =
		kv_list{{"AB", "2"}, {"AC", "3"}, {"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_between(kv, "A", "C", 5, kv_sort(expected));

	/* insert new key */
	ASSERT_STATUS(kv.put("BD", "7"), status::OK);

	expected = kv_list{{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_between(kv, "A", "C", 6, kv_sort(expected));

	expected = kv_list{{"BB", "5"}, {"BC", "6"}};
	verify_get_between(kv, "B", "BD", 2, kv_sort(expected));

	expected = kv_list{{"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_between(kv, "B", "BE", 3, kv_sort(expected));

	expected = kv_list{{"B", "4"}, {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_between(kv, "AZ", "BE", 4, kv_sort(expected));

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_between(kv, EMPTY_KEY, "ZZZ", 7, kv_sort(expected));

	verify_get_between(kv, MIN_KEY, MAX_KEY, 7, kv_sort(expected));

	/* insert new key with special char in key */
	ASSERT_STATUS(kv.put("记!", "RR"), status::OK);

	/* testing C-like API */
	expected = kv_list{{"BB", "5"}, {"BC", "6"}, {"BD", "7"}, {"记!", "RR"}};
	verify_get_between_c(kv, "B", MAX_KEY, 4, kv_sort(expected));

	expected = kv_list{{"BC", "6"}, {"BD", "7"}};
	verify_get_between_c(kv, "BB", "记!", 2, kv_sort(expected));

	expected = kv_list{{"BD", "7"}, {"记!", "RR"}};
	verify_get_between_c(kv, "BC", MAX_KEY, 2, kv_sort(expected));

	expected = kv_list{{"AB", "2"}, {"AC", "3"}, {"B", "4"},   {"BB", "5"},
			   {"BC", "6"}, {"BD", "7"}, {"记!", "RR"}};
	verify_get_between_c(kv, "AAA", "\xFF", 7, kv_sort(expected));

	/* empty/wrong range */
	verify_get_between_c(kv, EMPTY_KEY, EMPTY_KEY, 0, kv_list());
	verify_get_between_c(kv, "BB", "BB", 0, kv_list());
	verify_get_between_c(kv, "BX", "BX", 0, kv_list());
	verify_get_between_c(kv, "BA", "A", 0, kv_list());
	verify_get_between_c(kv, "记!", "BB", 0, kv_list());
	verify_get_between_c(kv, "记!", MIN_KEY, 0, kv_list());
	verify_get_between_c(kv, "记!", MAX_KEY, 0, kv_list());
	verify_get_between_c(kv, "ZZZ", "A", 0, kv_list());
	verify_get_between_c(kv, MAX_KEY, MIN_KEY, 0, kv_list());

	CLEAR_KV(kv);
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, 0, kv_list());

	kv.close();
}

static void GetBetweenTest2(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. Some keys are removed.
	 * This test is using C-like API.
	 * It's NOT suitable to test with custom comparator.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_ext_keys(kv);

	auto expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},  {"rrr", "4"},
				{"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_between_c(kv, EMPTY_KEY, "zzz", 7, kv_sort(expected));

	expected = kv_list{{"rrr", "4"}, {"sss", "5"}, {"ttt", "6"}};
	verify_get_between_c(kv, "ccc", "yyy", 3, kv_sort(expected));

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},  {"rrr", "4"},
			   {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_between_c(kv, "a", "z", 7, kv_sort(expected));

	expected = kv_list{{"rrr", "4"}, {"sss", "5"}, {"ttt", "6"}};
	verify_get_between_c(kv, "ddd", "yyy", 3, kv_sort(expected));

	expected = kv_list{
		{"aaa", "1"},
		{"bbb", "2"},
		{"ccc", "3"},
	};
	verify_get_between_c(kv, "a", "rrr", 3, kv_sort(expected));

	/* remove one key */
	ASSERT_STATUS(kv.remove("sss"), status::OK);

	expected = kv_list{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},
			   {"rrr", "4"}, {"ttt", "6"}, {"yyy", "记!"}};
	verify_get_between_c(kv, "a", "z", 6, kv_sort(expected));

	expected = kv_list{{"bbb", "2"}, {"ccc", "3"}, {"rrr", "4"}};
	verify_get_between_c(kv, "aaa", "sss", 3, kv_sort(expected));

	/* empty/wrong range */
	verify_get_between_c(kv, "yyy", "z", 0, kv_list());
	verify_get_between_c(kv, "yyyy", "z", 0, kv_list());
	verify_get_between_c(kv, "zzz", "zzzz", 0, kv_list());
	verify_get_between_c(kv, "z", "yyyy", 0, kv_list());
	verify_get_between_c(kv, "z", "yyy", 0, kv_list());
	verify_get_between_c(kv, MAX_KEY, MIN_KEY, 0, kv_list());

	CLEAR_KV(kv);
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, 0, kv_list());

	kv.close();
}

static void GetBetweenRandTest(std::string engine, pmem::kv::config &&config,
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
	verify_get_between(kv, MIN_KEY, "randtest", 0, kv_list());

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
		verify_get_between(kv, MIN_KEY, MAX_KEY, i + 1, kv_sort(expected));

		/* verifies elements above the first one (and below MAX_KEY) */
		auto exp_sorted = kv_sort(expected);
		verify_get_between(kv, exp_sorted[0].first, MAX_KEY, i,
				   kv_list(exp_sorted.begin() + 1, exp_sorted.end()));

		/* verifies elements below the last one (and above MIN_KEY) */
		verify_get_between(kv, MIN_KEY, exp_sorted[i].first, i,
				   kv_list(exp_sorted.begin(), exp_sorted.end() - 1));

		if (i > 1) {
			/* verifies elements between first and the last */
			verify_get_between(
				kv, exp_sorted[0].first, exp_sorted[i].first, i - 1,
				kv_list(exp_sorted.begin() + 1, exp_sorted.end() - 1));
		}

		if (exp_sorted.size() > 10) {
			/* verifies some elements in the middle */
			verify_get_between(
				kv, exp_sorted[4].first,
				exp_sorted[exp_sorted.size() - 5].first,
				exp_sorted.size() - 10,
				kv_list(exp_sorted.begin() + 5, exp_sorted.end() - 5));
		}
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetBetweenIncrTest(std::string engine, pmem::kv::config &&config,
			       const size_t max_key_len)
{
	/**
	 * TEST: Generated incremented keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added and checked if get_between returns properly all data.
	 * After initial part of the test, some new keys are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_between(kv, "a_inc", MAX_KEY, 0, kv_list());

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
		verify_get_between(kv, MIN_KEY, MAX_KEY, i + 1, kv_sort(expected));

		/* verifies elements above the first one (and below MAX_KEY) */
		auto exp_sorted = kv_sort(expected);
		verify_get_between(kv, exp_sorted[0].first, MAX_KEY, i,
				   kv_list(exp_sorted.begin() + 1, exp_sorted.end()));

		/* verifies elements below the last one (and above MIN_KEY) */
		verify_get_between(kv, MIN_KEY, exp_sorted[i].first, i,
				   kv_list(exp_sorted.begin(), exp_sorted.end() - 1));

		if (i > 10) {
			/* verifies some elements in the middle */
			verify_get_between(
				kv, exp_sorted[4].first,
				exp_sorted[exp_sorted.size() - 5].first, i - 9,
				kv_list(exp_sorted.begin() + 5, exp_sorted.end() - 5));
		}
	}

	/* start over with two initial keys */
	CLEAR_KV(kv);

	const std::string mid_key = std::string(2, char(127));
	ASSERT_STATUS(kv.put(mid_key, "init0"), status::OK);
	ASSERT_STATUS(kv.put(mid_key + mid_key, "init1"), status::OK);

	expected = kv_list{{mid_key, "init0"}, {mid_key + mid_key, "init1"}};
	verify_get_between(kv, MIN_KEY, MAX_KEY, 2, kv_sort(expected));

	/* add keys again */
	keys = gen_incr_keys(max_key_len);
	keys_cnt = charset_size * max_key_len;
	for (size_t i = 0; i < keys_cnt; i++) {
		key = keys[i];
		value = std::to_string(i);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		/* verifies all elements */
		verify_get_between(kv, MIN_KEY, MAX_KEY, i + 3, kv_sort(expected));

		/* verifies elements above the first one (and below MAX_KEY) */
		auto exp_sorted = kv_sort(expected);
		verify_get_between(kv, exp_sorted[0].first, MAX_KEY, i + 2,
				   kv_list(exp_sorted.begin() + 1, exp_sorted.end()));

		/* verifies elements below the last one (and above MIN_KEY) */
		verify_get_between(kv, MIN_KEY, exp_sorted[exp_sorted.size() - 1].first,
				   i + 2,
				   kv_list(exp_sorted.begin(), exp_sorted.end() - 1));

		if (i > 10) {
			/* verifies some elements in the middle */
			verify_get_between(
				kv, exp_sorted[4].first,
				exp_sorted[exp_sorted.size() - 5].first, i - 7,
				kv_list(exp_sorted.begin() + 5, exp_sorted.end() - 5));
		}
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetBetweenIncrReverseTest(std::string engine, pmem::kv::config &&config,
				      const size_t max_key_len)
{
	/**
	 * TEST: Generated incremented keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added in reverse order and checked if get_between returns properly all
	 * data. After initial part of the test, some keys are deleted and some new keys
	 * are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_between(kv, "&Rev&", "~~~", 0, kv_list());

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

		size_t curr_iter = keys_cnt - i;
		/* verifies all elements */
		verify_get_between(kv, MIN_KEY, MAX_KEY, curr_iter + 1,
				   kv_sort(expected));

		/* verifies elements above the first one (and below MAX_KEY) */
		auto exp_sorted = kv_sort(expected);
		verify_get_between(kv, exp_sorted[0].first, MAX_KEY, curr_iter,
				   kv_list(exp_sorted.begin() + 1, exp_sorted.end()));

		/* verifies elements below the last one (and above MIN_KEY) */
		verify_get_between(kv, MIN_KEY, exp_sorted[curr_iter].first, curr_iter,
				   kv_list(exp_sorted.begin(), exp_sorted.end() - 1));

		if (exp_sorted.size() > 10) {
			/* verifies some elements in the middle */
			verify_get_between(
				kv, exp_sorted[4].first,
				exp_sorted[exp_sorted.size() - 5].first,
				exp_sorted.size() - 10,
				kv_list(exp_sorted.begin() + 5, exp_sorted.end() - 5));
		}
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

	/* verifies above 11-th element */
	auto exp_sorted = kv_sort(expected);
	verify_get_between_c(kv, exp_sorted[10].first, MAX_KEY, keys_cnt - 11,
			     kv_list(exp_sorted.begin() + 11, exp_sorted.end()));

	/* verifies below 19-th element */
	verify_get_between_c(kv, MIN_KEY, exp_sorted[18].first, 18,
			     kv_list(exp_sorted.begin(), exp_sorted.begin() + 18));

	/* verifies between 11-th and 19-th elements */
	verify_get_between_c(kv, exp_sorted[10].first, exp_sorted[18].first, 7,
			     kv_list(exp_sorted.begin() + 11, exp_sorted.begin() + 18));

	/* verifies all elements */
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, keys_cnt, kv_sort(expected));

	/* remove 9th key */
	UT_ASSERT(keys_cnt > 9);
	key = keys[8];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies all elements */
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, keys_cnt, kv_sort(expected));

	/* remove 3rd key */
	UT_ASSERT(keys_cnt > 3);
	key = keys[2];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;

	/* verifies all elements */
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("!@", "!@"), status::OK);
	expected.emplace_back("!@", "!@");
	keys_cnt++;
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("<my_key>", "<my_key>"), status::OK);
	expected.emplace_back("<my_key>", "<my_key>");
	keys_cnt++;
	verify_get_between_c(kv, MIN_KEY, MAX_KEY, keys_cnt, kv_sort(expected));

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

	GetBetweenTest(engine, CONFIG_FROM_JSON(argv[2]));
	GetBetweenTest2(engine, CONFIG_FROM_JSON(argv[2]));
	GetBetweenRandTest(engine, CONFIG_FROM_JSON(argv[2]), items, max_key_len);
	GetBetweenIncrTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
	GetBetweenIncrReverseTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
