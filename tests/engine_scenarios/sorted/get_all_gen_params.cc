// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

/**
 * Basic + generated tests for get_all and count_all methods for sorted engines.
 * get_all method returns all elements in db (count returns the number of all records).
 */

static void GetAllTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: Basic test with hardcoded strings. New keys added, some keys removed.
	 * It's NOT suitable to test with custom comparator.
	 */
	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_all(kv, 0, kv_list());

	/* insert bunch of keys */
	add_basic_keys(kv);

	auto expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"},
				{"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_all(kv, 6, kv_sort(expected));

	/* insert new key */
	ASSERT_STATUS(kv.put("BD", "7"), status::OK);

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"}, {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_all(kv, 7, kv_sort(expected));

	/* insert new key */
	ASSERT_STATUS(kv.put("AA", "8"), status::OK);

	expected = kv_list{{"A", "1"}, {"AA", "8"}, {"AB", "2"}, {"AC", "3"},
			   {"B", "4"}, {"BB", "5"}, {"BC", "6"}, {"BD", "7"}};
	verify_get_all(kv, 8, kv_sort(expected));

	/* insert new key with special char in key */
	ASSERT_STATUS(kv.put("记!", "RR"), status::OK);

	expected =
		kv_list{{"A", "1"},  {"AA", "8"}, {"AB", "2"}, {"AC", "3"},  {"B", "4"},
			{"BB", "5"}, {"BC", "6"}, {"BD", "7"}, {"记!", "RR"}};
	verify_get_all(kv, 9, kv_sort(expected));

	/* insert bunch of new keys */
	add_ext_keys(kv);

	/* testing C-like API */
	expected = kv_list{{"A", "1"},	 {"AA", "8"},  {"AB", "2"},    {"AC", "3"},
			   {"B", "4"},	 {"BB", "5"},  {"BC", "6"},    {"BD", "7"},
			   {"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},   {"rrr", "4"},
			   {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}, {"记!", "RR"}};
	verify_get_all_c(kv, 16, kv_sort(expected));

	/* remove two keys */
	ASSERT_STATUS(kv.remove("A"), status::OK);
	ASSERT_STATUS(kv.remove("BC"), status::OK);

	/* testing C-like API */
	expected = kv_list{{"AA", "8"},	   {"AB", "2"},	 {"AC", "3"},  {"B", "4"},
			   {"BB", "5"},	   {"BD", "7"},	 {"aaa", "1"}, {"bbb", "2"},
			   {"ccc", "3"},   {"rrr", "4"}, {"sss", "5"}, {"ttt", "6"},
			   {"yyy", "记!"}, {"记!", "RR"}};
	verify_get_all_c(kv, 14, kv_sort(expected));

	CLEAR_KV(kv);
	kv.close();
}

static void GetAllRandTest(std::string engine, pmem::kv::config &&config,
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
	verify_get_all(kv, 0, kv_list());

	/* generate keys and put them one at a time */
	std::vector<std::string> keys = gen_rand_keys(items, max_key_len);
	auto expected = kv_list();
	std::string key, value;
	for (size_t i = 0; i < items; i++) {
		value = std::to_string(i);
		key = keys[i];
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		verify_get_all(kv, i + 1, kv_sort(expected));
	}

	CLEAR_KV(kv);
	kv.close();
}

static void GetAllIncrTest(std::string engine, pmem::kv::config &&config,
			   const size_t max_key_len)
{
	/**
	 * TEST: Generated incremented keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added and checked if get_all returns properly all data.
	 * After initial part of the test, some new keys are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_all(kv, 0, kv_list());

	/* generate keys and put them one at a time */
	std::vector<std::string> keys = gen_incr_keys(max_key_len);
	auto expected = kv_list();
	const size_t keys_cnt = charset_size * max_key_len;
	std::string key, value;
	for (size_t i = 0; i < keys_cnt; i++) {
		key = keys[i];
		value = std::to_string(i);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		verify_get_all(kv, i + 1, kv_sort(expected));
	}

	/* start over with 3 initial keys */
	CLEAR_KV(kv);
	const std::string mid_key = std::string(2, char(127));

	ASSERT_STATUS(kv.put(MIN_KEY, "init0"), status::OK);
	ASSERT_STATUS(kv.put(mid_key, "init1"), status::OK);
	ASSERT_STATUS(kv.put(MAX_KEY, "init2"), status::OK);
	expected = kv_list{{MIN_KEY, "init0"}, {mid_key, "init1"}, {MAX_KEY, "init2"}};

	/* testing C-like API */
	verify_get_all_c(kv, 3, kv_sort(expected));

	/* add keys again */
	keys = gen_incr_keys(max_key_len);
	for (size_t i = 0; i < keys_cnt; i++) {
		key = keys[i];
		value = std::to_string(i);
		ASSERT_STATUS(kv.put(key, value), status::OK);
		expected.emplace_back(key, value);

		verify_get_all(kv, i + 4, kv_sort(expected));
	}

	CLEAR_KV(kv);
	verify_get_all(kv, 0, kv_list());

	kv.close();
}

static void GetAllIncrReverseTest(std::string engine, pmem::kv::config &&config,
				  const size_t max_key_len)
{
	/**
	 * TEST: Generated incremented keys, e.g. "A", "AA", ..., "B", "BB", ...
	 * Keys are added in reverse order and checked if get_all returns properly all
	 * data. After initial part of the test, some keys are deleted and some new keys
	 * are added.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_all(kv, 0, kv_list());

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

		verify_get_all(kv, keys_cnt - i + 1, kv_sort(expected));
	}

	/* delete some keys, add some new keys and check again (using C-like API) */

	/* remove and check at 19th key */
	UT_ASSERT(keys_cnt > 20);
	key = keys[19];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;
	verify_get_all_c(kv, keys_cnt, kv_sort(expected));

	/* remove and check at 9th key */
	UT_ASSERT(keys_cnt > 9);
	key = keys[8];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;
	verify_get_all_c(kv, keys_cnt, kv_sort(expected));

	/* remove and check at 3rd key */
	UT_ASSERT(keys_cnt > 3);
	key = keys[2];
	ASSERT_STATUS(kv.get(key, &value), status::OK);
	ASSERT_STATUS(kv.remove(key), status::OK);
	expected.erase(std::remove(expected.begin(), expected.end(), kv_pair{key, value}),
		       expected.end());
	keys_cnt--;
	verify_get_all_c(kv, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("!@", "!@"), status::OK);
	expected.emplace_back("!@", "!@");
	keys_cnt++;
	verify_get_all_c(kv, keys_cnt, kv_sort(expected));

	ASSERT_STATUS(kv.put("<my_key>", "<my_key>"), status::OK);
	expected.emplace_back("<my_key>", "<my_key>");
	keys_cnt++;
	verify_get_all_c(kv, keys_cnt, kv_sort(expected));

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

	GetAllTest(engine, CONFIG_FROM_JSON(argv[2]));
	GetAllRandTest(engine, CONFIG_FROM_JSON(argv[2]), items, max_key_len);
	GetAllIncrTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
	GetAllIncrReverseTest(engine, CONFIG_FROM_JSON(argv[2]), max_key_len);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
