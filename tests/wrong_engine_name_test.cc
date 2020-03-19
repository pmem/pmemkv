// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include <libpmemkv.hpp>

#include "unittest.hpp"

static bool test_wrong_engine_name(std::string name)
{
	pmem::kv::db db;
	return db.open(name) == pmem::kv::status::WRONG_ENGINE_NAME;
}

static void errormsg_test()
{
	pmem::kv::db kv;
	auto s = kv.open("non-existing name");
	UT_ASSERT(s == pmem::kv::status::WRONG_ENGINE_NAME);

	auto err = pmem::kv::errormsg();
	UT_ASSERT(err.size() > 0);

	s = kv.open("non-existing name");
	UT_ASSERT(s == pmem::kv::status::WRONG_ENGINE_NAME);
	s = kv.open("non-existing name");
	UT_ASSERT(s == pmem::kv::status::WRONG_ENGINE_NAME);

	/* Test whether errormsg is cleared correctly after each error */
	UT_ASSERT(pmem::kv::errormsg() == err);

	kv.close();
}

int main()
{
	UT_ASSERT(test_wrong_engine_name("non_existent_name"));

#ifndef ENGINE_CMAP
	UT_ASSERT(test_wrong_engine_name("cmap"));
#endif

#ifndef ENGINE_VSMAP
	UT_ASSERT(test_wrong_engine_name("vsmap"));
#endif

#ifndef ENGINE_VCMAP
	UT_ASSERT(test_wrong_engine_name("vcmap"));
#endif

#ifndef ENGINE_TREE3
	UT_ASSERT(test_wrong_engine_name("tree3"));
#endif

#ifndef ENGINE_STREE
	UT_ASSERT(test_wrong_engine_name("stree"));
#endif

#ifndef ENGINE_CACHING
	UT_ASSERT(test_wrong_engine_name("caching"));
#endif

	errormsg_test();

	return 0;
}
