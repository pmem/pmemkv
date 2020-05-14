// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <ctime>
#include <limits.h>

using namespace pmem::kv;

static std::string PREFIX = "init";
static std::string SUFFIX = ";";
static std::string CLEAN_KEY_SUFFIX = "_clean";

static size_t CHARSET_RANGE_START = 0;
static size_t CHARSET_RANGE_END = (size_t)UCHAR_MAX;
static size_t CHARSET_LEN = CHARSET_RANGE_END - CHARSET_RANGE_START + 1;

void generate_binary_strings(std::vector<std::string> &strings, const size_t max_str_len,
			     const size_t cnt)
{
	for (size_t s = 0; s < cnt; ++s) {
		/* various lenght of string, min: 1 */
		size_t str_len = 1 + ((size_t)rand() % (max_str_len - 1));
		std::string gen_str;
		gen_str.reserve(str_len);
		for (size_t i = 0; i < str_len; i++) {
			gen_str.push_back(
				char(CHARSET_RANGE_START + (size_t)rand() % CHARSET_LEN));
		}
		strings.push_back(std::move(gen_str));
	}
}

static void BinaryKeyTest(pmem::kv::db &kv)
{
	/**
	 * TEST: each char from the char range is used in a single key
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);
	UT_ASSERTeq(kv.exists(PREFIX), status::NOT_FOUND);

	UT_ASSERTeq(kv.put(PREFIX, "should_not_change"), status::OK);
	UT_ASSERTeq(kv.exists(PREFIX), status::OK);
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 1);

	/* Add binary keys */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		std::string key = std::string(PREFIX + char(i) + SUFFIX);
		UT_ASSERTeq(kv.exists(key), status::NOT_FOUND);
		UT_ASSERTeq(kv.put(key, std::to_string(i)), status::OK);
	}

	std::string value;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == (CHARSET_LEN + 1));
	UT_ASSERT(kv.get(PREFIX, &value) == status::OK && value == "should_not_change");

	/* Read and remove binary keys */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		std::string key = std::string(PREFIX + char(i) + SUFFIX);

		UT_ASSERTeq(kv.exists(key), status::OK);
		UT_ASSERT(kv.get(key, &value) == status::OK &&
			  value == std::to_string(i));

		UT_ASSERTeq(kv.remove(key), status::OK);
		UT_ASSERTeq(kv.exists(key), status::NOT_FOUND);
	}

	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.get(PREFIX, &value) == status::OK && value == "should_not_change");
	UT_ASSERTeq(kv.remove(PREFIX), status::OK);
}

static void BinaryRandKeyTest(pmem::kv::db &kv)
{
	/**
	 * TEST: keys are randomly generated from the full range of the charset
	 */
	size_t KEYS_CNT = 16;
	size_t MAX_KEY_LEN = 16;

	size_t cnt = std::numeric_limits<size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);

	std::vector<std::string> keys;
	generate_binary_strings(keys, MAX_KEY_LEN, KEYS_CNT);

	/* Add elements with generated keys */
	for (size_t i = 0; i < KEYS_CNT; i++) {
		std::string istr = std::to_string(i);
		std::string key = istr + keys[i];
		UT_ASSERTeq(kv.put(key, istr), status::OK);
	}

	std::string value;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == KEYS_CNT);
	std::cout << "cnt: " + std::to_string(cnt) << std::endl;

	/* Read and remove elements with generated keys */
	for (size_t i = 0; i < KEYS_CNT; i++) {
		std::string istr = std::to_string(i);
		std::string key = istr + keys[i];

		UT_ASSERTeq(kv.exists(key), status::OK);
		UT_ASSERT(kv.get(key, &value) == status::OK && value == istr);

		UT_ASSERTeq(kv.remove(key), status::OK);
	}
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);
}

static void BinaryValueTest(pmem::kv::db &kv)
{
	/**
	 * TEST: each char from the char range is used as value -
	 * once with prefix and suffix, once just "as is" (clean value)
	 */
	size_t cnt = std::numeric_limits<size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);

	/* Add elements with binary values */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* value with prefix and suffix */
		std::string key1 = std::to_string(i);
		std::string value1 = std::string(PREFIX + char(i) + SUFFIX);
		UT_ASSERTeq(kv.exists(key1), status::NOT_FOUND);
		UT_ASSERTeq(kv.put(key1, value1), status::OK);

		/* "clean" value */
		std::string key2 = std::to_string(i) + CLEAN_KEY_SUFFIX;
		std::string value2 = std::string(1, char(i));
		UT_ASSERTeq(kv.exists(key2), status::NOT_FOUND);
		UT_ASSERTeq(kv.put(key2, value2), status::OK);
	}

	std::string value;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == (CHARSET_LEN * 2));

	/* Read and remove elements with binary values */
	for (auto i = CHARSET_RANGE_START; i <= CHARSET_RANGE_END; i++) {
		/* value with prefix and suffix */
		std::string key1 = std::to_string(i);
		std::string value1 = std::string(PREFIX + char(i) + SUFFIX);

		UT_ASSERTeq(kv.exists(key1), status::OK);
		UT_ASSERT(kv.get(key1, &value) == status::OK && value == value1);
		UT_ASSERTeq(value.length(), value1.length());
		UT_ASSERTeq(kv.remove(key1), status::OK);
		UT_ASSERTeq(kv.exists(key1), status::NOT_FOUND);

		/* "clean" value */
		std::string key2 = std::to_string(i) + CLEAN_KEY_SUFFIX;
		std::string value2 = std::string(1, char(i));

		UT_ASSERTeq(kv.exists(key2), status::OK);
		UT_ASSERT(kv.get(key2, &value) == status::OK && value == value2);
		UT_ASSERTeq(value.length(), value2.length());
		UT_ASSERTeq(kv.remove(key2), status::OK);
		UT_ASSERTeq(kv.exists(key2), status::NOT_FOUND);
	}

	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);
}

static void BinaryRandValueTest(pmem::kv::db &kv)
{
	/**
	 * TEST: values are randomly generated from the full range of the charset
	 */
	size_t ELEMENTS_CNT = 16;
	size_t MAX_VALUE_LEN = 16;

	size_t cnt = std::numeric_limits<size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);

	std::vector<std::string> values;
	generate_binary_strings(values, MAX_VALUE_LEN, ELEMENTS_CNT);

	/* Add elements with generated values */
	for (size_t i = 0; i < ELEMENTS_CNT; i++) {
		std::string key = std::to_string(i);
		std::string value = std::string(1, char(i));

		UT_ASSERTeq(kv.put(key, value), status::OK);
	}

	std::string value;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == ELEMENTS_CNT);

	/* Read and remove elements with generated values */
	for (size_t i = 0; i < ELEMENTS_CNT; i++) {
		std::string key = std::to_string(i);
		std::string exp_value = std::string(1, char(i));

		UT_ASSERT(kv.get(key, &value) == status::OK && value == exp_value);
		UT_ASSERTeq(value.length(), exp_value.length());
		UT_ASSERTeq(kv.remove(key), status::OK);
	}

	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 0);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto seed = unsigned(std::time(0));
	printf("rand seed: %u\n", seed);
	std::srand(seed);

	run_engine_tests(argv[1], argv[2],
			 {
				 BinaryKeyTest,
				 BinaryRandKeyTest,
				 BinaryValueTest,
				 BinaryRandValueTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
