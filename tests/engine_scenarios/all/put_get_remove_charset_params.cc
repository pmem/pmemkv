// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <ctime>
#include <limits.h>

using namespace pmem::kv;

static const std::string PREFIX = "init";
static const std::string SUFFIX = ";";
static const std::string CLEAN_KEY_SUFFIX = "_clean";

static constexpr size_t CHARSET_RANGE_START = 0;
static constexpr size_t CHARSET_RANGE_END = (size_t)UCHAR_MAX;
static constexpr size_t CHARSET_LEN = CHARSET_RANGE_END - CHARSET_RANGE_START + 1;

std::vector<std::string> generate_binary_strings(const size_t cnt,
						 const size_t max_str_len)
{
	std::vector<std::string> strings;
	for (size_t s = 0; s < cnt; ++s) {
		/* various lenght of string, min: 1 */
		size_t str_len = 1 + ((size_t)rand() % max_str_len);
		std::string gen_str;
		gen_str.reserve(str_len);
		for (size_t i = 0; i < str_len; i++) {
			gen_str.push_back(
				char(CHARSET_RANGE_START + (size_t)rand() % CHARSET_LEN));
		}
		strings.push_back(std::move(gen_str));
	}

	return strings;
}

static void BinaryKeyTest(pmem::kv::db &kv)
{
	/**
	 * TEST: each char from the char range is used in two keys
	 * once with prefix and suffix, once just as is ("clean key")
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
	ASSERT_STATUS(kv.exists(PREFIX), status::NOT_FOUND);

	ASSERT_STATUS(kv.put(PREFIX, "should_not_change"), status::OK);
	ASSERT_STATUS(kv.exists(PREFIX), status::OK);
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);

	/* Add binary keys */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* key with prefix and suffix */
		std::string key1 = std::string(PREFIX + char(i) + SUFFIX);
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key1, std::to_string(i)), status::OK);

		/* "clean" key */
		std::string key2 = std::string(1, char(i));
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key2, std::to_string(i)), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == (CHARSET_LEN * 2 + 1));
	ASSERT_STATUS(kv.get(PREFIX, &value), status::OK);
	UT_ASSERT(value == "should_not_change");

	/* Read and remove binary keys */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* key with prefix and suffix */
		std::string key1 = std::string(PREFIX + char(i) + SUFFIX);
		ASSERT_STATUS(kv.exists(key1), status::OK);
		ASSERT_STATUS(kv.get(key1, &value), status::OK);
		UT_ASSERT(value == std::to_string(i));

		ASSERT_STATUS(kv.remove(key1), status::OK);
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);

		/* "clean" key */
		std::string key2 = std::string(1, char(i));
		ASSERT_STATUS(kv.exists(key2), status::OK);
		ASSERT_STATUS(kv.get(key2, &value), status::OK);
		UT_ASSERT(value == std::to_string(i));

		ASSERT_STATUS(kv.remove(key2), status::OK);
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
	}

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get(PREFIX, &value), status::OK);
	UT_ASSERT(value == "should_not_change");
	ASSERT_STATUS(kv.remove(PREFIX), status::OK);
}

static void BinaryRandKeyTest(const size_t elements_cnt, const size_t max_key_len,
			      pmem::kv::db &kv)
{
	/**
	 * TEST: keys are randomly generated from the full range of the charset
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	std::vector<std::string> keys =
		generate_binary_strings(elements_cnt, max_key_len);

	/* Add elements with generated keys */
	for (size_t i = 0; i < elements_cnt; i++) {
		std::string istr = std::to_string(i);
		std::string key = istr + keys[i];
		ASSERT_STATUS(kv.put(key, istr), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == elements_cnt);

	/* Read and remove elements with generated keys (in reverse order) */
	for (size_t i = elements_cnt; i > 0; i--) {
		std::string istr = std::to_string(i - 1);
		std::string key = istr + keys[i - 1];

		ASSERT_STATUS(kv.exists(key), status::OK);
		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == istr);

		ASSERT_STATUS(kv.remove(key), status::OK);
	}
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
}

static void BinaryValueTest(pmem::kv::db &kv)
{
	/**
	 * TEST: each char from the char range is used as value twice -
	 * once with prefix and suffix, once just as is ("clean value")
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	/* Add elements with binary values */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* value with prefix and suffix */
		std::string key1 = std::to_string(i);
		std::string value1 = std::string(PREFIX + char(i) + SUFFIX);
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key1, value1), status::OK);

		/* "clean" value */
		std::string key2 = std::to_string(i) + CLEAN_KEY_SUFFIX;
		std::string value2 = std::string(1, char(i));
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
		ASSERT_STATUS(kv.put(key2, value2), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == (CHARSET_LEN * 2));

	/* Read and remove elements with binary values */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* value with prefix and suffix */
		std::string key1 = std::to_string(i);
		std::string value1 = std::string(PREFIX + char(i) + SUFFIX);

		ASSERT_STATUS(kv.exists(key1), status::OK);
		ASSERT_STATUS(kv.get(key1, &value), status::OK);
		UT_ASSERT(value == value1);
		UT_ASSERTeq(value.length(), value1.length());
		ASSERT_STATUS(kv.remove(key1), status::OK);
		ASSERT_STATUS(kv.exists(key1), status::NOT_FOUND);

		/* "clean" value */
		std::string key2 = std::to_string(i) + CLEAN_KEY_SUFFIX;
		std::string value2 = std::string(1, char(i));

		ASSERT_STATUS(kv.exists(key2), status::OK);
		ASSERT_STATUS(kv.get(key2, &value), status::OK);
		UT_ASSERT(value == value2);
		UT_ASSERTeq(value.length(), value2.length());
		ASSERT_STATUS(kv.remove(key2), status::OK);
		ASSERT_STATUS(kv.exists(key2), status::NOT_FOUND);
	}

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
}

static void BinaryRandValueTest(const size_t elements_cnt, const size_t max_value_len,
				pmem::kv::db &kv)
{
	/**
	 * TEST: values are randomly generated from the full range of the charset
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);

	std::vector<std::string> values =
		generate_binary_strings(elements_cnt, max_value_len);

	/* Add elements with generated values */
	for (size_t i = 0; i < elements_cnt; i++) {
		std::string key = std::to_string(i);
		std::string value = std::string(1, char(i));

		ASSERT_STATUS(kv.put(key, value), status::OK);
	}

	std::string value;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == elements_cnt);

	/* Read and remove elements with generated values (in reverse order) */
	for (size_t i = elements_cnt; i > 0; i--) {
		std::string key = std::to_string(i - 1);
		std::string exp_value = std::string(1, char(i - 1));

		ASSERT_STATUS(kv.get(key, &value), status::OK);
		UT_ASSERT(value == exp_value);
		UT_ASSERTeq(value.length(), exp_value.length());
		ASSERT_STATUS(kv.remove(key), status::OK);
	}

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 0);
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 5)
		UT_FATAL("usage: %s engine json_config elements_cnt max_str_len",
			 argv[0]);

	auto seed = unsigned(std::time(0));
	printf("rand seed: %u\n", seed);
	std::srand(seed);

	auto elements_cnt = std::stoull(argv[3]);
	auto max_str_len = std::stoull(argv[4]);
	run_engine_tests(
		argv[1], argv[2],
		{
			BinaryKeyTest,
			std::bind(BinaryRandKeyTest, elements_cnt, max_str_len, _1),
			BinaryValueTest,
			std::bind(BinaryRandValueTest, elements_cnt, max_str_len, _1),
		});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
