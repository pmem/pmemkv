// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "mock_tx_alloc.h"
#include "unittest.hpp"

const std::string LONGSTR =
	"123456789A123456789A123456789A123456789A123456789A123456789A123456789A";

const int init_iterations = 50000;

void validate(pmem::kv::db &kv)
{
	for (size_t i = 0; i < init_iterations; i++) {
		std::string istr = std::to_string(i);
		auto s = kv.get(istr, [&](pmem::kv::string_view val) {
			UT_ASSERT(val.compare(istr + "!") == 0);
		});
		ASSERT_STATUS(s, pmem::kv::status::OK);
	}
}

void populate(pmem::kv::db &kv)
{
	for (size_t i = 0; i < init_iterations; i++) {
		std::string istr = std::to_string(i);
		ASSERT_STATUS(kv.put(istr, (istr + "!")), pmem::kv::status::OK);
	}
}

void LongStringTest(pmem::kv::db &kv)
{
	populate(kv);

	ASSERT_STATUS(kv.remove("100"), pmem::kv::status::OK);
	tx_alloc_should_fail = true;
	ASSERT_STATUS(kv.put("100", LONGSTR), pmem::kv::status::OUT_OF_MEMORY);
	tx_alloc_should_fail = false;
	ASSERT_STATUS(kv.put("100", "100!"), pmem::kv::status::OK);

	validate(kv);
}

void ShortKeyTest(pmem::kv::db &kv)
{
	populate(kv);

	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		ASSERT_STATUS(kv.put("123456", LONGSTR), pmem::kv::status::OUT_OF_MEMORY);
	}
	tx_alloc_should_fail = false;
	ASSERT_STATUS(kv.remove("4567"), pmem::kv::status::OK);
	ASSERT_STATUS(kv.put("4567", "4567!"), pmem::kv::status::OK);

	validate(kv);
}

void LongKeyTest(pmem::kv::db &kv)
{
	populate(kv);

	tx_alloc_should_fail = true;
	for (int i = 0; i <= 99999; i++) {
		ASSERT_STATUS(kv.put(LONGSTR, "1"), pmem::kv::status::OUT_OF_MEMORY);
		ASSERT_STATUS(kv.put(LONGSTR, LONGSTR), pmem::kv::status::OUT_OF_MEMORY);
	}
	tx_alloc_should_fail = false;
	ASSERT_STATUS(kv.remove("34567"), pmem::kv::status::OK);
	ASSERT_STATUS(kv.put("34567", "34567!"), pmem::kv::status::OK);

	validate(kv);
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 LongStringTest,
				 ShortKeyTest,
				 LongKeyTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
