// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

/**
 * Basic tests for all count_* and get_* methods for sorted engines.
 */

using namespace pmem::kv;

inline void add_basic_keys(pmem::kv::db &kv)
{
	UT_ASSERTeq(kv.put("A", "1"), status::OK);
	UT_ASSERTeq(kv.put("AB", "2"), status::OK);
	UT_ASSERTeq(kv.put("AC", "3"), status::OK);
	UT_ASSERTeq(kv.put("B", "4"), status::OK);
	UT_ASSERTeq(kv.put("BB", "5"), status::OK);
	UT_ASSERTeq(kv.put("BC", "6"), status::OK);
}

inline void add_ext_keys(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);
}

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
	auto s = kv.get_above("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|");
	result.clear();

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);

	s = kv.get_above("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|BD,7|");
	result.clear();

	s = kv.get_above("", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|BD,7|");
	result.clear();

	s = kv.get_above("ZZZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();

	s = kv.get_above("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "BB,5|BC,6|BD,7|");
	result.clear();

	/* insert new key with special char in key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	/* testing C-like API */
	s = kv.get_above(
		"B",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&result);
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
	auto s = kv.get_above(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_above("a", cnt) == status::OK && cnt == 7);
	s = kv.get_above(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_above("ddd", cnt) == status::OK && cnt == 4);
	s = kv.get_above(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_above("z", cnt) == status::OK && cnt == 0);
	s = kv.get_above(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
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
	auto s = kv.get_equal_above("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|");
	result.clear();
	cnt = 0;

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);

	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 4);
	s = kv.get_equal_above("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|BD,7|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK && cnt == 7);
	s = kv.get_equal_above("", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|BD,7|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_equal_above("ZZZ", cnt) == status::OK && cnt == 0);
	s = kv.get_equal_above("ZZZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("AZ", cnt) == status::OK && cnt == 4);
	s = kv.get_equal_above("AZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "B,4|BB,5|BC,6|BD,7|");
	result.clear();
	cnt = 0;

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 5);
	/* testing C-like API */
	s = kv.get_equal_above(
		"B",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&result);
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
	auto s = kv.get_equal_above(
		"",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("ccc", cnt) == status::OK && cnt == 5);
	s = kv.get_equal_above(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 100;

	UT_ASSERT(kv.count_equal_above("a", cnt) == status::OK && cnt == 7);
	s = kv.get_equal_above(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("ddd", cnt) == status::OK);
	UT_ASSERTeq(4, cnt);

	s = kv.get_equal_above(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("x", cnt) == status::OK && cnt == 1);
	s = kv.get_equal_above(
		"x",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_above("yyy", cnt) == status::OK && cnt == 1);
	s = kv.get_equal_above(
		"yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<yyy>,<记!>|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_equal_above("z", cnt) == status::OK && cnt == 0);
	s = kv.get_equal_above(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
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
	auto s = kv.get_equal_below("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|AC,3|B,4|");
	result.clear();
	cnt = 0;

	/* insert new key */
	UT_ASSERTeq(kv.put("AA", "7"), status::OK);

	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK && cnt == 5);
	s = kv.get_equal_below("B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK && cnt == 0);
	s = kv.get_equal_below("", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 1024;

	UT_ASSERT(kv.count_equal_below("ZZZ", cnt) == status::OK && cnt == 7);
	s = kv.get_equal_below("ZZZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|");

	cnt = 10000;
	UT_ASSERT(kv.count_equal_below("AZ", cnt) == status::OK && cnt == 4);
	result.clear();
	s = kv.get_equal_below("AZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	UT_ASSERT(kv.count_equal_below("记!", cnt) == status::OK && cnt == 8);
	/* testing C-like API */
	s = kv.get_equal_below(
		"记!",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&result);
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
	auto s = kv.get_equal_below(
		"yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_equal_below("ttt", cnt) == status::OK && cnt == 6);
	s = kv.get_equal_below(
		"ttt",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result ==
		  "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");
	result.clear();
	cnt = 2048;

	UT_ASSERT(kv.count_equal_below("ccc", cnt) == status::OK && cnt == 3);
	s = kv.get_equal_below(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_below("z", cnt) == status::OK && cnt == 7);
	s = kv.get_equal_below(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_equal_below("ddd", cnt) == status::OK && cnt == 3);
	s = kv.get_equal_below(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");
	result.clear();
	cnt = 1;

	UT_ASSERT(kv.count_equal_below("a", cnt) == status::OK && cnt == 0);
	s = kv.get_equal_below(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 500;

	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK && cnt == 0);
	s = kv.get_equal_below(
		"",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
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
	auto s = kv.get_below("AC", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AB,2|");
	result.clear();

	/* insert new key */
	UT_ASSERTeq(kv.put("AA", "7"), status::OK);

	s = kv.get_below("AC", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|");
	result.clear();

	s = kv.get_below("", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();

	s = kv.get_below("ZZZZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|");
	result.clear();

	s = kv.get_below("AD", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|");
	result.clear();

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	/* testing C-like API */
	s = kv.get_below(
		"\xFF",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&result);
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
	auto s = kv.get_below(
		"a",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 8192;

	UT_ASSERT(kv.count_below("aaa", cnt) == status::OK && cnt == 0);
	s = kv.get_below(
		"aaa",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_below("ccc", cnt) == status::OK && cnt == 2);
	s = kv.get_below(
		"ccc",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|");
	result.clear();
	cnt = 1;

	UT_ASSERT(kv.count_below("ddd", cnt) == status::OK && cnt == 3);
	s = kv.get_below(
		"ddd",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");
	result.clear();
	cnt = 100000;

	UT_ASSERT(kv.count_below("x", cnt) == status::OK && cnt == 6);
	s = kv.get_below(
		"x",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result ==
		  "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_below("yyy", cnt) == status::OK && cnt == 6);
	s = kv.get_below(
		"yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result ==
		  "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_below("z", cnt) == status::OK && cnt == 7);
	s = kv.get_below(
		"z",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
}

static void GetBetweenTest(pmem::kv::db &kv)
{
	/**
	 * TEST: get_between method returns all elements in db with keys greater then
	 * first function's argument (key1) and lesser then second argument (key2).
	 */
	add_basic_keys(kv);

	std::string result;
	auto s = kv.get_between("A", "B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "AB,2|AC,3|");
	result.clear();

	/* insert new key */
	UT_ASSERTeq(kv.put("AA", "7"), status::OK);

	s = kv.get_between("A", "B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "AA,7|AB,2|AC,3|");
	result.clear();

	s = kv.get_between("", "ZZZ", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|B,4|BB,5|BC,6|");
	result.clear();

	s = kv.get_between("", "A", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("", "B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "A,1|AA,7|AB,2|AC,3|");
	result.clear();

	s = kv.get_between("", "", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("A", "A", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("AC", "A", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("B", "A", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("BD", "A", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("ZZZ", "A", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());

	s = kv.get_between("A", "B", [&](string_view k, string_view v) {
		result.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "AA,7|AB,2|AC,3|");
	result.clear();

	/* insert new key */
	UT_ASSERT(kv.put("记!", "RR") == status::OK);

	/* testing C-like API */
	s = kv.get_between(
		"B", "\xFF",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&result);
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
	auto s = kv.get_between(
		"", "rrr",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");
	result.clear();
	cnt = 0;

	UT_ASSERT(kv.count_between("ccc", "ttt", cnt) == status::OK && cnt == 2);
	s = kv.get_between(
		"ccc", "ttt",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|");
	result.clear();
	cnt = 2;

	UT_ASSERT(kv.count_between("ddd", "x", cnt) == status::OK && cnt == 3);
	s = kv.get_between(
		"ddd", "x",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");
	result.clear();
	cnt = 5;

	UT_ASSERT(kv.count_between("aaa", "yyy", cnt) == status::OK && cnt == 5);
	s = kv.get_between(
		"aaa", "yyy",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result == "<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");
	result.clear();
	cnt = std::numeric_limits<std::size_t>::max();

	UT_ASSERT(kv.count_between("yyy", "zzz", cnt) == status::OK && cnt == 0);
	s = kv.get_between(
		"yyy", "zzz",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(result.empty());
	result.clear();
	cnt = 100;

	UT_ASSERT(kv.count_between("", "zzz", cnt) == status::OK && cnt == 7);
	s = kv.get_between(
		"", "zzz",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
	result.clear();
	cnt = 128;

	UT_ASSERT(kv.count_between("", "", cnt) == status::OK && cnt == 0);
	s = kv.get_between(
		"", "",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append("<");
			c->append(std::string(k, kb));
			c->append(">,<");
			c->append(std::string(v, vb));
			c->append(">|");

			return 0;
		},
		&result);
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
