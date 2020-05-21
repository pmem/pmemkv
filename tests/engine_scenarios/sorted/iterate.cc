// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

/**
 * Basic tests for all count_* and get_* methods for sorted engines.
 */

static void CountTest(pmem::kv::db &kv)
{
	/**
	 * TEST: all count_* methods with basic keys (without any special chars in keys)
	 */
	add_basic_keys(kv);

	std::size_t cnt;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 6);

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 7);

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_above("", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_above("A", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_above("B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_above("BC", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_above("BD", cnt) == status::OK && cnt == 0);
	cnt = 1;
	UT_ASSERT(kv.count_above("ZZ", cnt) == status::OK && cnt == 0);

	cnt = 0;
	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK && cnt == 7);
	cnt = 0;
	UT_ASSERT(kv.count_equal_above("A", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_equal_above("AA", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 4);
	UT_ASSERT(kv.count_equal_above("BC", cnt) == status::OK && cnt == 2);
	UT_ASSERT(kv.count_equal_above("BD", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_equal_above("Z", cnt) == status::OK && cnt == 0);

	cnt = 1;
	UT_ASSERT(kv.count_below("", cnt) == status::OK && cnt == 0);
	cnt = 10;
	UT_ASSERT(kv.count_below("A", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_below("B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_below("BC", cnt) == status::OK && cnt == 5);
	UT_ASSERT(kv.count_below("BD", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_below("ZZZZZ", cnt) == status::OK && cnt == 7);

	cnt = 256;
	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_equal_below("A", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK && cnt == 4);
	cnt = 257;
	UT_ASSERT(kv.count_equal_below("BA", cnt) == status::OK && cnt == 4);
	UT_ASSERT(kv.count_equal_below("BC", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_equal_below("BD", cnt) == status::OK && cnt == 7);
	cnt = 258;
	UT_ASSERT(kv.count_equal_below("ZZZZZZ", cnt) == status::OK && cnt == 7);

	cnt = 1024;
	UT_ASSERT(kv.count_between("", "ZZZZ", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_between("", "A", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_between("", "B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_between("A", "B", cnt) == status::OK && cnt == 2);
	UT_ASSERT(kv.count_between("A", "BD", cnt) == status::OK && cnt == 5);
	UT_ASSERT(kv.count_between("B", "ZZ", cnt) == status::OK && cnt == 3);

	cnt = 1024;
	UT_ASSERT(kv.count_between("", "", cnt) == status::OK && cnt == 0);
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
}

static void GetAboveTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_above method returns all elements in db with greater keys
	 */
	add_basic_keys(kv);

	std::string result;
	auto s = KV_GET_1KEY_CPP_CB(get_above, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|");
	result.clear();

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);

	s = KV_GET_1KEY_CPP_CB(get_above, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|BD,7|");
	result.clear();

	s = KV_GET_1KEY_CPP_CB(get_above, "", result);
	;
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|BD,7|");
	result.clear();

	s = KV_GET_1KEY_CPP_CB(get_above, "ZZZ", result);
	;
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();

	s = KV_GET_1KEY_CPP_CB(get_above, "BA", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|BD,7|");
	result.clear();

	/* insert new key with special char in key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	/* testing C-like API */
	s = KV_GET_1KEY_C_CB(get_above, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|BD,7|记!,RR|");
}

static void GetAboveTest2(pmem::kv::db &kv)
{
	/**
	 * TEST: get_above method returns all elements in db with greater keys.
	 * This test uses C-like API. It also uses count_above method.
	 */
	add_ext_keys(kv);

	std::size_t cnt;
	std::string result;
	UT_ASSERT(kv.count_above("ccc", cnt) == status::OK && cnt == 4);
	auto s = KV_GET_1KEY_C_CB(get_above, "ccc", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_above("a", cnt) == status::OK && cnt == 7);
	s = KV_GET_1KEY_C_CB(get_above, "a", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_above("ddd", cnt) == status::OK && cnt == 4);
	s = KV_GET_1KEY_C_CB(get_above, "ddd", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_above("z", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_C_CB(get_above, "z", result);
	;
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
}

static void GetEqualAboveTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_equal_above method returns all elements in db with greater or equal
	 * keys. This test also uses count_equal_above method.
	 */
	add_basic_keys(kv);

	std::string result;
	std::size_t cnt;
	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 3);
	auto s = KV_GET_1KEY_CPP_CB(get_equal_above, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|");
	result.clear();
	cnt = 0;

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);

	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 4);
	s = KV_GET_1KEY_CPP_CB(get_equal_above, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|BD,7|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK && cnt == 7);
	s = KV_GET_1KEY_CPP_CB(get_equal_above, "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|BD,7|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_equal_above("ZZZ", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_CPP_CB(get_equal_above, "ZZZ", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("AZ", cnt) == status::OK && cnt == 4);
	s = KV_GET_1KEY_CPP_CB(get_equal_above, "AZ", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|BD,7|");
	result.clear();
	cnt = 0;

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 5);
	/* testing C-like API */
	s = KV_GET_1KEY_C_CB(get_equal_above, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|BD,7|记!,RR|");
}

static void GetEqualAboveTest2(pmem::kv::db &kv)
{
	/**
	 * TEST: get_equal_above method returns all elements in db with greater or equal
	 * keys. This test uses C-like API. It also uses count_equal_above method.
	 */
	add_ext_keys(kv);

	std::string result;
	std::size_t cnt;
	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK && cnt == 7);
	auto s = KV_GET_1KEY_C_CB(get_equal_above, "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("ccc", cnt) == status::OK && cnt == 5);
	s = KV_GET_1KEY_C_CB(get_equal_above, "ccc", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 100;

	UT_ASSERT(kv.count_equal_above("a", cnt) == status::OK && cnt == 7);
	s = KV_GET_1KEY_C_CB(get_equal_above, "a", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("ddd", cnt) == status::OK && cnt == 4);
	s = KV_GET_1KEY_C_CB(get_equal_above, "ddd", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("x", cnt) == status::OK && cnt == 1);
	s = KV_GET_1KEY_C_CB(get_equal_above, "x", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("yyy", cnt) == status::OK && cnt == 1);
	s = KV_GET_1KEY_C_CB(get_equal_above, "yyy", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "yyy,记!|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_equal_above("z", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_C_CB(get_equal_above, "z", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
}

static void GetEqualBelowTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_equal_below method returns all elements in db with lesser or equal
	 * keys. This test also uses count_equal_below method.
	 */
	add_basic_keys(kv);

	std::string result;
	std::size_t cnt;
	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK && cnt == 4);
	auto s = KV_GET_1KEY_CPP_CB(get_equal_below, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|AC,3|B,4|");
	result.clear();
	cnt = 0;

	/* insert new key */
	UT_ASSERTeq(kv.put("AA", "7"), status::OK);

	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK && cnt == 5);
	s = KV_GET_1KEY_CPP_CB(get_equal_below, "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_CPP_CB(get_equal_below, "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 1024;

	UT_ASSERT(kv.count_equal_below("ZZZ", cnt) == status::OK && cnt == 7);
	s = KV_GET_1KEY_CPP_CB(get_equal_below, "ZZZ", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|");
	result.clear();
	cnt = 10000;

	UT_ASSERT(kv.count_equal_below("AZ", cnt) == status::OK && cnt == 4);
	s = KV_GET_1KEY_CPP_CB(get_equal_below, "AZ", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	UT_ASSERT(kv.count_equal_below("记!", cnt) == status::OK && cnt == 8);
	/* testing C-like API */
	s = KV_GET_1KEY_C_CB(get_equal_below, "记!", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

static void GetEqualBelowTest2(pmem::kv::db &kv)
{
	/**
	 * TEST: get_equal_below method returns all elements in db with lesser or equal
	 * keys. This test uses C-like API. It also uses count_equal_below method.
	 */
	add_ext_keys(kv);

	std::string result;
	std::size_t cnt;
	UT_ASSERT(kv.count_equal_below("yyy", cnt) == status::OK && cnt == 7);
	auto s = KV_GET_1KEY_C_CB(get_equal_below, "yyy", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_equal_below("ttt", cnt) == status::OK && cnt == 6);
	s = KV_GET_1KEY_C_CB(get_equal_below, "ttt", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|");
	result.clear();
	cnt = 2048;

	UT_ASSERT(kv.count_equal_below("ccc", cnt) == status::OK && cnt == 3);
	s = KV_GET_1KEY_C_CB(get_equal_below, "ccc", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_below("z", cnt) == status::OK && cnt == 7);
	s = KV_GET_1KEY_C_CB(get_equal_below, "z", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_below("ddd", cnt) == status::OK && cnt == 3);
	s = KV_GET_1KEY_C_CB(get_equal_below, "ddd", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|");
	result.clear();
	cnt = 1;

	UT_ASSERT(kv.count_equal_below("a", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_C_CB(get_equal_below, "a", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 500;

	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_C_CB(get_equal_below, "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
}

static void GetBelowTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_below method returns all elements in db with lesser keys.
	 */
	add_basic_keys(kv);

	std::string result;
	auto s = KV_GET_1KEY_CPP_CB(get_below, "AC", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|");
	result.clear();

	/* insert new key */
	UT_ASSERTeq(kv.put("AA", "7"), status::OK);

	s = KV_GET_1KEY_CPP_CB(get_below, "AC", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|");
	result.clear();

	s = KV_GET_1KEY_CPP_CB(get_below, "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();

	s = KV_GET_1KEY_CPP_CB(get_below, "ZZZZ", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|");
	result.clear();

	s = KV_GET_1KEY_CPP_CB(get_below, "AD", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|");
	result.clear();

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	/* testing C-like API */
	s = KV_GET_1KEY_C_CB(get_below, "\xFF", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

static void GetBelowTest2(pmem::kv::db &kv)
{
	/**
	 * TEST: get_below method returns all elements in db with lesser keys.
	 * This test uses C-like API. It also uses count_below method.
	 */
	add_ext_keys(kv);

	std::string result;
	std::size_t cnt;
	UT_ASSERT(kv.count_below("a", cnt) == status::OK && cnt == 0);
	auto s = KV_GET_1KEY_C_CB(get_below, "a", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 8192;

	UT_ASSERT(kv.count_below("aaa", cnt) == status::OK && cnt == 0);
	s = KV_GET_1KEY_C_CB(get_below, "aaa", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_below("ccc", cnt) == status::OK && cnt == 2);
	s = KV_GET_1KEY_C_CB(get_below, "ccc", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|");
	result.clear();
	cnt = 1;

	UT_ASSERT(kv.count_below("ddd", cnt) == status::OK && cnt == 3);
	s = KV_GET_1KEY_C_CB(get_below, "ddd", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|");
	result.clear();
	cnt = 100000;

	UT_ASSERT(kv.count_below("x", cnt) == status::OK && cnt == 6);
	s = KV_GET_1KEY_C_CB(get_below, "x", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_below("yyy", cnt) == status::OK && cnt == 6);
	s = KV_GET_1KEY_C_CB(get_below, "yyy", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_below("z", cnt) == status::OK && cnt == 7);
	s = KV_GET_1KEY_C_CB(get_below, "z", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
}

static void GetBetweenTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_between method returns all elements in db with keys greater then
	 * first function's argument (key1) and lesser then second argument (key2).
	 */
	add_basic_keys(kv);

	std::string result;
	auto s = KV_GET_2KEYS_CPP_CB(get_between, "A", "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "AB,2|AC,3|");
	result.clear();

	/* insert new key */
	UT_ASSERTeq(kv.put("AA", "7"), status::OK);

	s = KV_GET_2KEYS_CPP_CB(get_between, "A", "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "AA,7|AB,2|AC,3|");
	result.clear();

	s = KV_GET_2KEYS_CPP_CB(get_between, "", "ZZZ", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|");
	result.clear();

	s = KV_GET_2KEYS_CPP_CB(get_between, "", "A", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "", "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|");
	result.clear();

	s = KV_GET_2KEYS_CPP_CB(get_between, "", "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "A", "A", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "AC", "A", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "B", "A", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "BD", "A", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "ZZZ", "A", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = KV_GET_2KEYS_CPP_CB(get_between, "A", "B", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "AA,7|AB,2|AC,3|");
	result.clear();

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	/* testing C-like API */
	s = KV_GET_2KEYS_C_CB(get_between, "B", "\xFF", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|记!,RR|");
}

static void GetBetweenTest2(pmem::kv::db &kv)
{
	/**
	 * TEST: get_between method returns all elements in db with keys greater then
	 * first function's argument (key1) and lesser then second argument (key2).
	 * This test uses C-like API. It also uses count_between method.
	 */
	add_ext_keys(kv);

	std::string result;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("", "rrr", cnt) == status::OK && cnt == 3);
	auto s = KV_GET_2KEYS_C_CB(get_between, "", "rrr", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_between("ccc", "ttt", cnt) == status::OK && cnt == 2);
	s = KV_GET_2KEYS_C_CB(get_between, "ccc", "ttt", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "rrr,4|sss,5|");
	result.clear();
	cnt = 2;

	UT_ASSERT(kv.count_between("ddd", "x", cnt) == status::OK && cnt == 3);
	s = KV_GET_2KEYS_C_CB(get_between, "ddd", "x", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "rrr,4|sss,5|ttt,6|");
	result.clear();
	cnt = 5;

	UT_ASSERT(kv.count_between("aaa", "yyy", cnt) == status::OK && cnt == 5);
	s = KV_GET_2KEYS_C_CB(get_between, "aaa", "yyy", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "bbb,2|ccc,3|rrr,4|sss,5|ttt,6|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_between("yyy", "zzz", cnt) == status::OK && cnt == 0);
	s = KV_GET_2KEYS_C_CB(get_between, "yyy", "zzz", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 100;

	UT_ASSERT(kv.count_between("", "zzz", cnt) == status::OK && cnt == 7);
	s = KV_GET_2KEYS_C_CB(get_between, "", "zzz", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "aaa,1|bbb,2|ccc,3|rrr,4|sss,5|ttt,6|yyy,记!|");
	result.clear();
	cnt = 128;

	UT_ASSERT(kv.count_between("", "", cnt) == status::OK && cnt == 0);
	s = KV_GET_2KEYS_C_CB(get_between, "", "", result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
}

int main(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	return run_engine_tests(argv[1], argv[2],
				{
					CountTest,
					GetAboveTest,
					GetEqualAboveTest,
					GetEqualBelowTest,
					GetBelowTest,
					GetBetweenTest,
					GetAboveTest2,
					GetEqualAboveTest2,
					GetEqualBelowTest2,
					GetBelowTest2,
					GetBetweenTest2,
				});
}
