// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <limits>

using namespace pmem::kv;

static const char *EXPECTED_ERR_MSG =
	"[pmemkv_open] Comparator with name: \"__pmemkv_binary_comparator\" expected";

class invalid_comparator {
public:
	int compare(string_view k1, string_view k2)
	{
		return k2.compare(k1);
	}

	std::string name()
	{
		return "invalid_cmp";
	}
};

static void insert(std::string name, pmem::kv::config &&cfg)
{
	pmem::kv::db kv;
	auto s = kv.open(name, std::move(cfg));
	ASSERT_STATUS(s, status::OK);

	s = kv.put("A", "A");
	ASSERT_STATUS(s, status::OK);
	s = kv.put("B", "B");
	ASSERT_STATUS(s, status::OK);
	s = kv.put("C", "C");
	ASSERT_STATUS(s, status::OK);
	s = kv.put("D", "D");
	ASSERT_STATUS(s, status::OK);

	kv.close();
}

static void check_valid(std::string name, pmem::kv::config &&cfg)
{
	pmem::kv::db kv;
	auto s = kv.open(name, std::move(cfg));
	ASSERT_STATUS(s, status::OK);

	size_t cnt = std::numeric_limits<size_t>::max();
	s = kv.count_above("B", cnt);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(cnt, 2);

	cnt = std::numeric_limits<size_t>::max();
	s = kv.count_below("B", cnt);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERTeq(cnt, 1);
	kv.close();
}

static void check_invalid(std::string name, pmem::kv::config &&cfg)
{
	auto s = cfg.put_comparator(invalid_comparator{});
	ASSERT_STATUS(s, status::OK);

	pmem::kv::db kv;
	s = kv.open(name, std::move(cfg));
	ASSERT_STATUS(s, status::COMPARATOR_MISMATCH);

	UT_ASSERT(pmem::kv::errormsg() == EXPECTED_ERR_MSG);
}

static void test(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s engine json_config insert/check", argv[0]);

	std::string engine = argv[1];
	std::string json_config = argv[2];
	std::string mode = argv[3];
	if (mode != "insert" && mode != "check")
		UT_FATAL("usage: %s engine json_config insert/check", argv[0]);

	if (mode == "insert") {
		insert(engine, CONFIG_FROM_JSON(json_config));
	} else {
		check_valid(engine, CONFIG_FROM_JSON(json_config));
		check_invalid(engine, CONFIG_FROM_JSON(json_config));
	}
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
