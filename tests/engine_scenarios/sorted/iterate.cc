// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

/**
 * Common tests for all count_* and get_* methods for sorted engines.
 */

static void CommonCountTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: mix of all count_* methods with hardcoded strings.
	 * It's NOT suitable to test with custom comparator.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));

	/* insert bunch of keys */
	add_basic_keys(kv);

	std::size_t cnt;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 6);

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 7);

	cnt = std::numeric_limits<std::size_t>::max();
	/* mixed count functions checked with empty key */
	UT_ASSERT(kv.count_above(EMPTY_KEY, cnt) == status::OK && cnt == 7);
	cnt = 0;
	UT_ASSERT(kv.count_equal_above(EMPTY_KEY, cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_below(EMPTY_KEY, cnt) == status::OK && cnt == 0);
	cnt = 256;
	UT_ASSERT(kv.count_equal_below(EMPTY_KEY, cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_between(EMPTY_KEY, "ZZZZ", cnt) == status::OK && cnt == 7);
	cnt = 10000;
	UT_ASSERT(kv.count_between(EMPTY_KEY, MAX_KEY, cnt) == status::OK && cnt == 7);

	/* group of check for each count_* function, tested with various keys */
	UT_ASSERT(kv.count_above("A", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_above("B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_above("BC", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_above("BD", cnt) == status::OK && cnt == 0);
	cnt = 1;
	UT_ASSERT(kv.count_above("ZZ", cnt) == status::OK && cnt == 0);

	cnt = 0;
	UT_ASSERT(kv.count_equal_above("A", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_equal_above("AA", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 4);
	UT_ASSERT(kv.count_equal_above("BC", cnt) == status::OK && cnt == 2);
	UT_ASSERT(kv.count_equal_above("BD", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_equal_above("Z", cnt) == status::OK && cnt == 0);

	cnt = 10;
	UT_ASSERT(kv.count_below("A", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_below("B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_below("BC", cnt) == status::OK && cnt == 5);
	UT_ASSERT(kv.count_below("BD", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_below("ZZZZZ", cnt) == status::OK && cnt == 7);

	UT_ASSERT(kv.count_equal_below("A", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK && cnt == 4);
	cnt = 257;
	UT_ASSERT(kv.count_equal_below("BA", cnt) == status::OK && cnt == 4);
	UT_ASSERT(kv.count_equal_below("BC", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_equal_below("BD", cnt) == status::OK && cnt == 7);
	cnt = 258;
	UT_ASSERT(kv.count_equal_below("ZZZZZZ", cnt) == status::OK && cnt == 7);

	cnt = 1024;
	UT_ASSERT(kv.count_between(EMPTY_KEY, "A", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_between(EMPTY_KEY, "B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_between("A", "B", cnt) == status::OK && cnt == 2);
	UT_ASSERT(kv.count_between("A", "BD", cnt) == status::OK && cnt == 5);
	UT_ASSERT(kv.count_between("B", "ZZ", cnt) == status::OK && cnt == 3);

	cnt = 1024;
	UT_ASSERT(kv.count_between(EMPTY_KEY, EMPTY_KEY, cnt) == status::OK && cnt == 0);
	cnt = 1025;
	UT_ASSERT(kv.count_between("A", "A", cnt) == status::OK && cnt == 0);
	cnt = 1026;
	UT_ASSERT(kv.count_between("AC", "A", cnt) == status::OK && cnt == 0);
	cnt = 1027;
	UT_ASSERT(kv.count_between("B", "A", cnt) == status::OK && cnt == 0);
	cnt = 1028;
	UT_ASSERT(kv.count_between("BD", "A", cnt) == status::OK && cnt == 0);
	cnt = 1029;
	UT_ASSERT(kv.count_between("ZZZ", "B", cnt) == status::OK && cnt == 0);

	CLEAR_KV(kv);
	kv.close();
}

static void CommonGetTest(std::string engine, pmem::kv::config &&config)
{
	/**
	 * TEST: mix of all get_* and count_* methods with hardcoded strings.
	 * It's NOT suitable to test with custom comparator.
	 */

	auto kv = INITIALIZE_KV(engine, std::move(config));
	verify_get_all(kv, 0, kv_list());
	verify_get_above(kv, EMPTY_KEY, 0, kv_list());
	verify_get_below(kv, EMPTY_KEY, 0, kv_list());
	verify_get_between(kv, MIN_KEY, MAX_KEY, 0, kv_list());
	verify_get_equal_above(kv, EMPTY_KEY, 0, kv_list());
	verify_get_equal_below(kv, EMPTY_KEY, 0, kv_list());

	/* insert bunch of keys */
	add_basic_keys(kv);

	auto expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"},
				{"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_below(kv, MAX_KEY, 6, kv_sort(expected));
	verify_get_equal_above(kv, EMPTY_KEY, 6, kv_sort(expected));
	verify_get_all(kv, 6, kv_sort(expected));
	verify_get_above(kv, EMPTY_KEY, 6, kv_sort(expected));
	verify_get_equal_below(kv, MAX_KEY, 6, kv_sort(expected));
	verify_get_between(kv, EMPTY_KEY, MAX_KEY, 6, kv_sort(expected));

	/* insert new key with special char in key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	expected = kv_list{{"A", "1"},	{"AB", "2"}, {"AC", "3"},  {"B", "4"},
			   {"BB", "5"}, {"BC", "6"}, {"记!", "RR"}};
	verify_get_between(kv, EMPTY_KEY, MAX_KEY, expected.size(), kv_sort(expected));
	verify_get_equal_above(kv, EMPTY_KEY, expected.size(), kv_sort(expected));
	verify_get_above(kv, EMPTY_KEY, expected.size(), kv_sort(expected));
	verify_get_below(kv, MAX_KEY, expected.size(), kv_sort(expected));
	verify_get_equal_below(kv, MAX_KEY, expected.size(), kv_sort(expected));
	verify_get_all(kv, expected.size(), kv_sort(expected));

	/* remove the new key */
	UT_ASSERT(kv.remove("记!") == status::OK);

	expected = kv_list{{"A", "1"}, {"AB", "2"}, {"AC", "3"},
			   {"B", "4"}, {"BB", "5"}, {"BC", "6"}};
	verify_get_below(kv, "Z", 6, kv_sort(expected));
	verify_get_between(kv, EMPTY_KEY, "Z", 6, kv_sort(expected));
	verify_get_equal_below(kv, "Z", 6, kv_sort(expected));
	verify_get_above(kv, EMPTY_KEY, 6, kv_sort(expected));
	verify_get_all(kv, 6, kv_sort(expected));
	verify_get_equal_above(kv, EMPTY_KEY, 6, kv_sort(expected));

	CLEAR_KV(kv);
	verify_get_equal_above(kv, EMPTY_KEY, 0, kv_list());
	verify_get_all(kv, 0, kv_list());
	verify_get_above(kv, EMPTY_KEY, 0, kv_list());
	verify_get_equal_below(kv, EMPTY_KEY, 0, kv_list());
	verify_get_between(kv, MIN_KEY, MAX_KEY, 0, kv_list());
	verify_get_below(kv, EMPTY_KEY, 0, kv_list());

	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto engine = std::string(argv[1]);
	CommonCountTest(engine, CONFIG_FROM_JSON(argv[2]));
	CommonGetTest(engine, CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
