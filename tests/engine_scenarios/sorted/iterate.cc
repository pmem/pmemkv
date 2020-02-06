// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void UsesCountTest(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("A", "1") == status::OK);
	UT_ASSERT(kv.put("AB", "2") == status::OK);
	UT_ASSERT(kv.put("AC", "3") == status::OK);
	UT_ASSERT(kv.put("B", "4") == status::OK);
	UT_ASSERT(kv.put("BB", "5") == status::OK);
	UT_ASSERT(kv.put("BC", "6") == status::OK);
	UT_ASSERT(kv.put("BD", "7") == status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 7);

	UT_ASSERT(kv.count_above("", cnt) == status::OK);
	UT_ASSERT(cnt == 7);
	UT_ASSERT(kv.count_above("A", cnt) == status::OK);
	UT_ASSERT(cnt == 6);
	UT_ASSERT(kv.count_above("B", cnt) == status::OK);
	UT_ASSERT(cnt == 3);
	UT_ASSERT(kv.count_above("BC", cnt) == status::OK);
	UT_ASSERT(cnt == 1);
	UT_ASSERT(kv.count_above("BD", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_above("Z", cnt) == status::OK);
	UT_ASSERT(cnt == 0);

	UT_ASSERT(kv.count_below("", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_below("A", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_below("B", cnt) == status::OK);
	UT_ASSERT(cnt == 3);
	UT_ASSERT(kv.count_below("BD", cnt) == status::OK);
	UT_ASSERT(cnt == 6);
	UT_ASSERT(kv.count_below("ZZZZZ", cnt) == status::OK);
	UT_ASSERT(cnt == 7);

	UT_ASSERT(kv.count_between("", "ZZZZ", cnt) == status::OK);
	UT_ASSERT(cnt == 7);
	UT_ASSERT(kv.count_between("", "A", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_between("", "B", cnt) == status::OK);
	UT_ASSERT(cnt == 3);
	UT_ASSERT(kv.count_between("A", "B", cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	UT_ASSERT(kv.count_between("B", "ZZZZ", cnt) == status::OK);
	UT_ASSERT(cnt == 3);

	UT_ASSERT(kv.count_between("", "", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_between("A", "A", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_between("AC", "A", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_between("B", "A", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_between("BD", "A", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.count_between("ZZZ", "B", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
}

static void UsesGetAllAboveTest(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("A", "1") == status::OK);
	UT_ASSERT(kv.put("AB", "2") == status::OK);
	UT_ASSERT(kv.put("AC", "3") == status::OK);
	UT_ASSERT(kv.put("B", "4") == status::OK);
	UT_ASSERT(kv.put("BB", "5") == status::OK);
	UT_ASSERT(kv.put("BC", "6") == status::OK);

	std::string x;
	kv.get_above("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "BB,5|BC,6|");

	x = "";
	kv.get_above("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	x = "";
	kv.get_above("ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x.empty());

	x = "";
	kv.get_above("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "BB,5|BC,6|");

	UT_ASSERT(kv.put("记!", "RR") == status::OK);
	x = "";
	kv.get_above(
		"B",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	UT_ASSERT(x == "BB,5|BC,6|记!,RR|");
}

static void UsesGetAllAboveTest2(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_above("ccc", cnt) == status::OK);
	UT_ASSERTeq(4, cnt);
	std::string result;
	kv.get_above(
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
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_above("a", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	result.clear();
	kv.get_above(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_above("ddd", cnt) == status::OK);
	UT_ASSERTeq(4, cnt);
	result.clear();
	kv.get_above(
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
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_above("z", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	result.clear();
	kv.get_above(
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
	UT_ASSERT(result.empty());
}

static void UsesGetAllEqualAboveTest(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("A", "1") == status::OK);
	UT_ASSERT(kv.put("AB", "2") == status::OK);
	UT_ASSERT(kv.put("AC", "3") == status::OK);
	UT_ASSERT(kv.put("B", "4") == status::OK);
	UT_ASSERT(kv.put("BB", "5") == status::OK);
	UT_ASSERT(kv.put("BC", "6") == status::OK);

	std::string x;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	kv.get_equal_above("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "B,4|BB,5|BC,6|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK);
	UT_ASSERTeq(6, cnt);
	x = "";
	kv.get_equal_above("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("ZZZ", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	x = "";
	kv.get_equal_above("ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("AZ", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	x = "";
	kv.get_equal_above("AZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "B,4|BB,5|BC,6|");

	UT_ASSERT(kv.put("记!", "RR") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK);
	UT_ASSERTeq(4, cnt);
	x = "";
	kv.get_equal_above(
		"B",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	UT_ASSERT(x == "B,4|BB,5|BC,6|记!,RR|");
}

static void UsesGetAllEqualAboveTest2(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	std::string result;
	kv.get_equal_above(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("ccc", cnt) == status::OK);
	UT_ASSERTeq(5, cnt);
	result.clear();
	kv.get_equal_above(
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
	UT_ASSERT(result == "<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("a", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	result.clear();
	kv.get_equal_above(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("ddd", cnt) == status::OK);
	UT_ASSERTeq(4, cnt);
	result.clear();
	kv.get_equal_above(
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
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("x", cnt) == status::OK);
	UT_ASSERTeq(1, cnt);
	result.clear();
	kv.get_equal_above(
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
	UT_ASSERT(result == "<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("yyy", cnt) == status::OK);
	UT_ASSERTeq(1, cnt);
	result.clear();
	kv.get_equal_above(
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
	UT_ASSERT(result == "<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("z", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	result.clear();
	kv.get_equal_above(
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
	UT_ASSERT(result.empty());
}

static void UsesGetAllEqualBelowTest(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("A", "1") == status::OK);
	UT_ASSERT(kv.put("AB", "2") == status::OK);
	UT_ASSERT(kv.put("AC", "3") == status::OK);
	UT_ASSERT(kv.put("B", "4") == status::OK);
	UT_ASSERT(kv.put("BB", "5") == status::OK);
	UT_ASSERT(kv.put("BC", "6") == status::OK);

	std::string x;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK);
	UT_ASSERTeq(4, cnt);
	kv.get_equal_below("B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	x = "";
	kv.get_equal_below("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("ZZZ", cnt) == status::OK);
	UT_ASSERTeq(6, cnt);
	x = "";
	kv.get_equal_below("ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("AZ", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	x = "";
	kv.get_equal_below("AZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|");

	UT_ASSERT(kv.put("记!", "RR") == status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("记!", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	x = "";
	kv.get_equal_below(
		"记!",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

static void UsesGetAllEqualBelowTest2(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("yyy", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	std::string result;
	kv.get_equal_below(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("ttt", cnt) == status::OK);
	UT_ASSERTeq(6, cnt);
	result.clear();
	kv.get_equal_below(
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
	UT_ASSERT(result ==
		  "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("ccc", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	result.clear();
	kv.get_equal_below(
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
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("z", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	result.clear();
	kv.get_equal_below(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("ddd", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	result.clear();
	kv.get_equal_below(
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
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("a", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	result.clear();
	kv.get_equal_below(
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
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	result.clear();
	kv.get_equal_below(
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
	UT_ASSERT(result.empty());
}

static void UsesGetAllBelowTest(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("A", "1") == status::OK);
	UT_ASSERT(kv.put("AB", "2") == status::OK);
	UT_ASSERT(kv.put("AC", "3") == status::OK);
	UT_ASSERT(kv.put("B", "4") == status::OK);
	UT_ASSERT(kv.put("BB", "5") == status::OK);
	UT_ASSERT(kv.put("BC", "6") == status::OK);

	std::string x;
	kv.get_below("AC", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|");

	x = "";
	kv.get_below("", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x.empty());

	x = "";
	kv.get_below("ZZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	x = "";
	kv.get_below("AC", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|");

	UT_ASSERT(kv.put("记!", "RR") == status::OK);
	x = "";
	kv.get_below(
		"\xFF",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|记!,RR|");
}

static void UsesGetAllBelowTest2(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("a", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	std::string result;
	kv.get_below(
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
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("aaa", cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	result.clear();
	kv.get_below(
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
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("ccc", cnt) == status::OK);
	UT_ASSERT(cnt == 2);
	result.clear();
	kv.get_below(
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
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("ddd", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	result.clear();
	kv.get_below(
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
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("x", cnt) == status::OK);
	UT_ASSERTeq(6, cnt);
	result.clear();
	kv.get_below(
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
	UT_ASSERT(result ==
		  "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("yyy", cnt) == status::OK);
	UT_ASSERTeq(6, cnt);
	result.clear();
	kv.get_below(
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
	UT_ASSERT(result ==
		  "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("z", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	result.clear();
	kv.get_below(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");
}

static void UsesGetAllBetweenTest(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("A", "1") == status::OK);
	UT_ASSERT(kv.put("AB", "2") == status::OK);
	UT_ASSERT(kv.put("AC", "3") == status::OK);
	UT_ASSERT(kv.put("B", "4") == status::OK);
	UT_ASSERT(kv.put("BB", "5") == status::OK);
	UT_ASSERT(kv.put("BC", "6") == status::OK);

	std::string x;
	kv.get_between("A", "B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "AB,2|AC,3|");

	x = "";
	kv.get_between("", "ZZZ", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|B,4|BB,5|BC,6|");

	x = "";
	kv.get_between("", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x.empty());

	x = "";
	kv.get_between("", "B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "A,1|AB,2|AC,3|");

	x = "";
	kv.get_between("", "", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv.get_between("A", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv.get_between("AC", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv.get_between("B", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv.get_between("BD", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	kv.get_between("ZZZ", "A", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x.empty());

	x = "";
	kv.get_between("A", "B", [&](string_view k, string_view v) {
		x.append(k.data(), k.size())
			.append(",")
			.append(v.data(), v.size())
			.append("|");
		return 0;
	});
	UT_ASSERT(x == "AB,2|AC,3|");

	UT_ASSERT(kv.put("记!", "RR") == status::OK);
	x = "";
	kv.get_between(
		"B", "\xFF",
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			const auto c = ((std::string *)arg);
			c->append(std::string(k, kb))
				.append(",")
				.append(std::string(v, vb))
				.append("|");
			return 0;
		},
		&x);
	UT_ASSERT(x == "BB,5|BC,6|记!,RR|");
}

static void UsesGetAllBetweenTest2(pmem::kv::db &kv)
{

	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);

	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("", "rrr", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	std::string result;
	kv.get_between(
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
	UT_ASSERT(result == "<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("ccc", "ttt", cnt) == status::OK);
	UT_ASSERTeq(2, cnt);
	result.clear();
	kv.get_between(
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
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("ddd", "x", cnt) == status::OK);
	UT_ASSERTeq(3, cnt);
	result.clear();
	kv.get_between(
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
	UT_ASSERT(result == "<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("aaa", "yyy", cnt) == status::OK);
	UT_ASSERTeq(5, cnt);
	result.clear();
	kv.get_between(
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
	UT_ASSERT(result == "<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("yyy", "zzz", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	result.clear();
	kv.get_between(
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
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("", "zzz", cnt) == status::OK);
	UT_ASSERTeq(7, cnt);
	result.clear();
	kv.get_between(
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
	UT_ASSERT(
		result ==
		"<aaa>,<1>|<bbb>,<2>|<ccc>,<3>|<rrr>,<4>|<sss>,<5>|<ttt>,<6>|<yyy>,<记!>|");

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("", "", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	result.clear();
	kv.get_between(
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
	UT_ASSERT(result.empty());
}

int main(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	return run_engine_tests(argv[1], argv[2],
				{
					UsesCountTest,
					UsesGetAllAboveTest,
					UsesGetAllEqualAboveTest,
					UsesGetAllEqualBelowTest,
					UsesGetAllBelowTest,
					UsesGetAllBetweenTest,
					UsesGetAllAboveTest2,
					UsesGetAllEqualAboveTest2,
					UsesGetAllEqualBelowTest2,
					UsesGetAllBelowTest2,
					UsesGetAllBetweenTest2,
				});
}
