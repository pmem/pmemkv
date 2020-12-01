// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * result.cpp -- test if result returns values/statuses properly.
 */

#include <libpmemobj++/slice.hpp>

#include "../common/unittest.hpp"
#include "libpmemkv.hpp"

using slice = pmem::obj::slice<const char *>;
template <typename T>
using result = pmem::kv::result<T>;

static void result_basic_test()
{
	const std::string str = "abcdefgh";

	/* result with correct value */
	result<slice> res1(slice(&str[0], &str[str.size()]));
	UT_ASSERT(res1.is_ok());
	ASSERT_STATUS(res1.get_status(), pmem::kv::status::OK);
	UT_ASSERT(res1 == pmem::kv::status::OK);
	UT_ASSERTeq(str.compare(res1.get_value().begin()), 0);

	/* result without value */
	for (int i = 1; i < 13; ++i) {
		result<slice> res2(static_cast<pmem::kv::status>(i));
		UT_ASSERT(!res2.is_ok());
		UT_ASSERTeq(res2.get_status(), static_cast<pmem::kv::status>(i));
		UT_ASSERT(res2 == static_cast<pmem::kv::status>(i));

		bool exception_thrown = false;
		try {
			res2.get_value();
		} catch (pmem::kv::bad_result_access &e) {
			exception_thrown = true;
		}
		UT_ASSERT(exception_thrown);
	}
}

/* counter of destructor calls */
class d_counter {
public:
	static size_t cnt;

	~d_counter()
	{
		++d_counter::cnt;
	}
};

size_t d_counter::cnt;

/* test if destructors are properly called */
static void destructor_test()
{
	d_counter c;
	{
		result<d_counter> r(c);
	}
	/* test if value inside of the result was destroyed */
	UT_ASSERTeq(d_counter::cnt, 1);

	{
		result<d_counter> r1(c);
		result<d_counter> r2(c);
		r1 = r2;
		/* check if the value in r1 was destroyed during a copy assignment */
		UT_ASSERTeq(d_counter::cnt, 2);
	}
	/* check if values in r1 and r2 were destroyed */
	UT_ASSERTeq(d_counter::cnt, 4);
}

static void test(int argc, char *argv[])
{
	result_basic_test();
	destructor_test();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
