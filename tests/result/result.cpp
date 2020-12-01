// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemobj++/slice.hpp>

#include "../common/unittest.hpp"
#include "libpmemkv.hpp"

using slice = pmem::obj::slice<const char *>;
using result = pmem::kv::result<slice, pmem::kv::status>;

static void test(int argc, char *argv[])
{
	const std::string str = "abcdefgh";

	result res1(slice(&str[0], &str[str.size()]));
	UT_ASSERT(res1.is_ok());
	UT_ASSERT(!res1.is_error());
	UT_ASSERTeq(str.compare(res1.ok().begin()), 0);

	bool exception_thrown = false;
	try {
		res1.err();
	} catch (pmem::kv::bad_result_access &e) {
		exception_thrown = true;
	}
	UT_ASSERT(exception_thrown);

	result res2(pmem::kv::status::NOT_FOUND);
	UT_ASSERT(!res2.is_ok());
	UT_ASSERT(res2.is_error());
	ASSERT_STATUS(res2.err(), pmem::kv::status::NOT_FOUND);

	exception_thrown = false;
	try {
		res2.ok();
	} catch (pmem::kv::bad_result_access &e) {
		exception_thrown = true;
	}
	UT_ASSERT(exception_thrown);
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
