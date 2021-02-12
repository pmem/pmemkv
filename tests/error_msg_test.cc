// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include <libpmemkv.hpp>

#include "unittest.hpp"

static void errormsg_cleared()
{
	pmem::kv::db kv;
	auto s = kv.open("non-existing name");
	UT_ASSERT(s == pmem::kv::status::WRONG_ENGINE_NAME);

	auto err = pmem::kv::errormsg();
	UT_ASSERT(err.size() > 0);

	s = kv.open("blackhole");
	UT_ASSERT(s == pmem::kv::status::OK);

	std::string value;
	s = kv.get("Nonexisting key:", &value);
	UT_ASSERT(s == pmem::kv::status::NOT_FOUND);
	err = pmem::kv::errormsg();
	UT_ASSERT(err == "");
	UT_ASSERTeq(err.size(), 0);

	s = kv.open("non-existing name");
	UT_ASSERT(s == pmem::kv::status::WRONG_ENGINE_NAME);
	err = pmem::kv::errormsg();
	UT_ASSERT(err.size() > 0);
}

int main()
{
	errormsg_cleared();

	return 0;
}
