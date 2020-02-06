// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

#include "../src/libpmemkv.hpp"
#include <cassert>

bool test_wrong_engine_name(std::string name)
{
	pmem::kv::db db;
	return db.open(name) == pmem::kv::status::WRONG_ENGINE_NAME;
}

int main()
{
	assert(test_wrong_engine_name("non_existent_name"));

#ifndef ENGINE_CMAP
	assert(test_wrong_engine_name("cmap"));
#endif

#ifndef ENGINE_VSMAP
	assert(test_wrong_engine_name("vsmap"));
#endif

#ifndef ENGINE_VCMAP
	assert(test_wrong_engine_name("vcmap"));
#endif

#ifndef ENGINE_TREE3
	assert(test_wrong_engine_name("tree3"));
#endif

#ifndef ENGINE_STREE
	assert(test_wrong_engine_name("stree"));
#endif

#ifndef ENGINE_CACHING
	assert(test_wrong_engine_name("caching"));
#endif

	return 0;
}
