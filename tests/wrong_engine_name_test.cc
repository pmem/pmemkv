// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#include <libpmemkv.hpp>

#include "unittest.hpp"

/**
 * Tests if open() returns WRONG_ENGINE_NAME if wrong engine's name is passed.
 */

static bool wrong_engine_name_test(std::string name)
{
	/**
	 * TEST: if engine is not switched on (in CMake) it should not open
	 */
	pmem::kv::db db;
	return db.open(name) == pmem::kv::status::WRONG_ENGINE_NAME;
}

static void errormsg_test()
{
	/**
	 * TEST: using WRONG_ENGINE_NAME status, we check if errormsg is properly set
	 */
	pmem::kv::db kv;
	auto s = kv.open("non-existing name");
	ASSERT_STATUS(s, pmem::kv::status::WRONG_ENGINE_NAME);

	auto err = pmem::kv::errormsg();
	UT_ASSERT(err.size() > 0);

	s = kv.open("non-existing name");
	ASSERT_STATUS(s, pmem::kv::status::WRONG_ENGINE_NAME);
	s = kv.open("non-existing name");
	ASSERT_STATUS(s, pmem::kv::status::WRONG_ENGINE_NAME);

	/* Test whether errormsg is cleared correctly after each error */
	UT_ASSERT(pmem::kv::errormsg() == err);

	/* Test if instance of db reports the same error */
	UT_ASSERT(kv.errormsg() == err);

	kv.close();
}

int main()
{
	UT_ASSERT(wrong_engine_name_test("non_existent_name"));

#ifndef ENGINE_CMAP
	UT_ASSERT(wrong_engine_name_test("cmap"));
#endif

#ifndef ENGINE_VSMAP
	UT_ASSERT(wrong_engine_name_test("vsmap"));
#endif

#ifndef ENGINE_VCMAP
	UT_ASSERT(wrong_engine_name_test("vcmap"));
#endif

#ifndef ENGINE_CSMAP
	UT_ASSERT(wrong_engine_name_test("csmap"));
#endif

#ifndef ENGINE_TREE3
	UT_ASSERT(wrong_engine_name_test("tree3"));
#endif

#ifndef ENGINE_STREE
	UT_ASSERT(wrong_engine_name_test("stree"));
#endif

#ifndef ENGINE_RADIX
	UT_ASSERT(wrong_engine_name_test("radix"));
#endif

#ifndef ENGINE_ROBINHOOD
	UT_ASSERT(wrong_engine_name_test("robinhood"));
#endif
#ifndef ENGINE_DRAM_VCMAP
	UT_ASSERT(wrong_engine_name_test("dram_vcmap"));
#endif

	errormsg_test();

	return 0;
}
